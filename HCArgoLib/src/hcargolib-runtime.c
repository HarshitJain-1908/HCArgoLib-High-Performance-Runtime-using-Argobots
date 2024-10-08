#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include "hclib.h"
#include "abt.h"

/*
    Use of ABT_mutex instead of pthread_mutex while doing pop/steal lowers the context switch
    overhead since dealing in user space resulting in better performance.

    The runtime dumps the number of asyncs, pops & steals whose updation require use of locks
    by the execution streams that affects the execution time, to get a better performance remove
    those updation and statistics.

    Asyncs, pops & steals counters aren't made volatile intentionally to avoid its overhead
    as these counts are for our own understanding so need not be exact.
*/


int NUM_XSTREAMS;
ABT_xstream *xstreams = NULL;
ABT_sched *scheds = NULL;
ABT_pool *pools = NULL;
static double user_specified_timer = 0;

int steals = 0, pops = 0, push = 0;
pthread_mutex_t mutex;

//=========================================== POOL STRUCTURE AND OPERATIONS =====================================================

typedef struct unit_t unit_t;
typedef struct pool_t pool_t;

struct unit_t {
    unit_t *p_prev;
    unit_t *p_next;
    ABT_thread thread;
};

struct pool_t {
    ABT_mutex lock;
    unit_t *p_head;
    unit_t *p_tail;
};

static ABT_unit pool_create_unit(ABT_pool pool, ABT_thread thread)
{
    unit_t *p_unit = (unit_t *)calloc(1, sizeof(unit_t));
    if (!p_unit)
        return ABT_UNIT_NULL;
    p_unit->thread = thread;
    return (ABT_unit)p_unit;
}

static void pool_free_unit(ABT_pool pool, ABT_unit unit)
{
    unit_t *p_unit = (unit_t *)unit;
    free(p_unit);
}

static ABT_bool pool_is_empty(ABT_pool pool)
{
    pool_t *p_pool;
    ABT_pool_get_data(pool, (void **)&p_pool);
    return p_pool->p_head ? ABT_FALSE : ABT_TRUE;
}

static ABT_thread pop_ULT_from_pool(ABT_pool pool, ABT_pool_context context)
{
    pool_t *p_pool;

    //get the handle of pool
    ABT_pool_get_data(pool, (void **)&p_pool);

    unit_t *p_unit = NULL;

    ABT_mutex_lock(p_pool->lock);

    if (p_pool->p_head == NULL){
        // No ULT to execute
    }else if(p_pool->p_head == p_pool->p_tail){
        // Only one ULT inside the pool
        p_unit = p_pool->p_head;
        p_pool->p_head = NULL;
        p_pool->p_tail = NULL;
    }else if(context & ABT_POOL_CONTEXT_OWNER_SECONDARY) {
        // Thief steals ULT from the victim
        p_unit = p_pool->p_tail;
        p_pool->p_tail = p_unit->p_next;
    }else{
        // Victim pops the ULT
        p_unit = p_pool->p_head;
        p_pool->p_head = p_unit->p_prev;
    }

    ABT_mutex_unlock(p_pool->lock);

    if (!p_unit)
        return ABT_THREAD_NULL;

    //returning the popped/stolen ULT
    return p_unit->thread;
}

static void push_ULT_in_pool(ABT_pool pool, ABT_unit unit, ABT_pool_context context)
{
    pool_t *p_pool;

    //get the handle of pool
    ABT_pool_get_data(pool, (void **)&p_pool);

    unit_t *p_unit = (unit_t *)unit;

    // Lockless push to the pool

    if (p_pool->p_head){
        p_unit->p_prev = p_pool->p_head;
        p_pool->p_head->p_next = p_unit;
    }else{
        p_pool->p_tail = p_unit;
    }

    p_pool->p_head = p_unit;
}

static int pool_init(ABT_pool pool, ABT_pool_config config)
{
    pool_t *p_pool = (pool_t *)calloc(1, sizeof(pool_t));

    if (!p_pool)
        return ABT_ERR_MEM;

    ABT_mutex_create(&p_pool->lock);

    ABT_pool_set_data(pool, (void *)p_pool);
    return ABT_SUCCESS;
}

static void pool_free(ABT_pool pool)
{
    pool_t *p_pool;
    ABT_pool_get_data(pool, (void **)&p_pool);

    ABT_mutex_free(&p_pool->lock);
    free(p_pool);
}

static void create_pools(int num, ABT_pool *pools)
{
    ABT_pool_user_def def;
    ABT_pool_user_def_create(pool_create_unit, pool_free_unit, pool_is_empty,
                             pop_ULT_from_pool, push_ULT_in_pool, &def);
    ABT_pool_user_def_set_init(def, pool_init);
    ABT_pool_user_def_set_free(def, pool_free);

    ABT_pool_config config;
    ABT_pool_config_create(&config);

    const int automatic = 1;
    ABT_pool_config_set(config, ABT_pool_config_automatic.key,
                        ABT_pool_config_automatic.type, &automatic);

    for (int i = 0; i < num; i++) {
        ABT_pool_create(def, config, &pools[i]);
    }
    ABT_pool_user_def_free(&def);
    ABT_pool_config_free(&config);
}

//=======================================================  SCHEDULER STRUCTURE AND CONFIGURATION =====================================================

typedef struct {
    uint32_t event_freq;
} sched_data_t;

static int sched_init(ABT_sched sched, ABT_sched_config config)
{
    sched_data_t *p_data = (sched_data_t *)calloc(1, sizeof(sched_data_t));

    ABT_sched_config_read(config, 1, &p_data->event_freq);
    ABT_sched_set_data(sched, (void *)p_data);

    return ABT_SUCCESS;
}

static void sched_run(ABT_sched sched)
{
    uint32_t work_count = 0;
    sched_data_t *p_data;
    int num_pools;
    ABT_pool *pools;
    int victim_rank;
    ABT_bool stop;
    unsigned seed = time(NULL);

    ABT_sched_get_data(sched, (void **)&p_data);
    ABT_sched_get_num_pools(sched, &num_pools);
    pools = (ABT_pool *)malloc(num_pools * sizeof(ABT_pool));
    ABT_sched_get_pools(sched, num_pools, 0, pools);

    while (1) {

        //Grab ULT from the pools and then execute

        ABT_thread thread;

        //First try to find from its own pool i.e. pop
        ABT_pool_pop_thread(pools[0], &thread);
        if (thread != ABT_THREAD_NULL) {
            ABT_self_schedule(thread, ABT_POOL_NULL);
            // ABT_mutex_lock(mutex);
            pthread_mutex_lock(&mutex);
            pops++;
            pthread_mutex_unlock(&mutex);
            // ABT_mutex_unlock(mutex);
            
        }else if (num_pools > 1) {
            //In case pop fails, try to steal from other pools

            victim_rank = rand_r(&seed) % (num_pools - 1) + 1;

            ABT_pool_pop_thread(pools[victim_rank], &thread);
            if (thread != ABT_THREAD_NULL) {
                ABT_self_schedule(thread, pools[victim_rank]);
                // ABT_mutex_lock(mutex);
                pthread_mutex_lock(&mutex);
                steals++;
                pthread_mutex_unlock(&mutex);
                // ABT_mutex_unlock(mutex);
                
                // int rank;
                // ABT_self_get_xstream_rank(&rank);
                // printf("task stolen by ES: %d\n", rank);
            }
        }

        if (++work_count >= p_data->event_freq) {
            work_count = 0;
            ABT_sched_has_to_stop(sched, &stop);
            if (stop == ABT_TRUE)
                break;
            ABT_xstream_check_events(sched);
        }
    }

    free(pools);
}

static int sched_free(ABT_sched sched)
{
    sched_data_t *p_data;

    ABT_sched_get_data(sched, (void **)&p_data);
    free(p_data);

    return ABT_SUCCESS;
}

static void create_scheds(int num, ABT_pool *pools, ABT_sched *scheds)
{
    ABT_sched_config config;
    ABT_pool *my_pools;

    ABT_sched_config_var cv_event_freq = { .idx = 0, .type = ABT_SCHED_CONFIG_INT };

    ABT_sched_def sched_def = { .type = ABT_SCHED_TYPE_ULT, .init = sched_init, .run = sched_run, .free = sched_free, .get_migr_pool = NULL };

    ABT_sched_config_create(&config, cv_event_freq, 10, ABT_sched_config_var_end);

    my_pools = (ABT_pool *)malloc(num * sizeof(ABT_pool));

    for (int i = 0; i < num; i++) {
        for (int k = 0; k < num; k++) {
            my_pools[k] = pools[(i + k) % num];
        }

        ABT_sched_create(&sched_def, num, my_pools, config, &scheds[i]);
    }

    free(my_pools);

    ABT_sched_config_free(&config);
}

double mysecond() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}

//=========================================================  MAIN FUNCTIONS ====================================================

void hclib_init(int argc, char *argv[]) {
    NUM_XSTREAMS = (getenv("HCLIB_WORKERS") != NULL) ? atoi(getenv("HCLIB_WORKERS")) : 4;
    xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * NUM_XSTREAMS);
    pools = (ABT_pool *)malloc(sizeof(ABT_pool) * NUM_XSTREAMS);
    scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * NUM_XSTREAMS);
    
    ABT_init(argc, argv);

    pthread_mutex_init(&mutex, NULL);
    // ABT_mutex_create(&mutex);

    create_pools(NUM_XSTREAMS, pools);

    create_scheds(NUM_XSTREAMS, pools, scheds);

    ABT_xstream_self(&xstreams[0]);
    ABT_xstream_set_main_sched(xstreams[0], scheds[0]);
    
    for (int i = 1; i < NUM_XSTREAMS; i++) {
        ABT_xstream_create(scheds[i], &xstreams[i]);
    }

    printf("\n====== HCArgoLib Runtime Initialized ======\n\n");
    printf("HCLIB_WORKERS=%d\n", NUM_XSTREAMS);
}

void hclib_finish(generic_frame_ptr fct_ptr, void * arg) {
    fct_ptr(arg);
}

void hclib_async(generic_frame_ptr fct_ptr, void * arg){
    ABT_xstream xstream;
    int rank;

    ABT_xstream_self(&xstream);
    ABT_xstream_get_rank(xstream, &rank);

    ABT_thread *threads = (ABT_thread *)malloc(sizeof(ABT_thread));

    ABT_thread_create(pools[rank], fct_ptr, (void *)arg, ABT_THREAD_ATTR_NULL, &threads);

    // ABT_mutex_lock(mutex);
    pthread_mutex_lock(&mutex);
    push++;
    pthread_mutex_unlock(&mutex);
    // ABT_mutex_unlock(mutex);
    
    ABT_thread_free(&threads);
}


void hclib_finalize() {
    for (int i = 1; i < NUM_XSTREAMS; i++) {
        ABT_xstream_join(xstreams[i]);
        ABT_xstream_free(&xstreams[i]);
    }
    for (int i = 1; i < NUM_XSTREAMS; i++) {
        ABT_sched_free(&scheds[i]);
    }

    pthread_mutex_destroy(&mutex);
    // ABT_mutex_free(&mutex);

    ABT_finalize();

    printf("\n=========== Tabulate Statistics ===========\n");
    printf("\nKernel execution time = %.3fs\n",user_specified_timer);
    printf("Total asyncs: %d, Total pops: %d, Total steals: %d\n", push, pops, steals);
    printf("\n======= HCArgoLib Runtime Finalized =======\n\n");
}

void hclib_kernel(generic_frame_ptr fct_ptr, void * arg) {
    printf("Executing kernel..\n");

    double start = mysecond();
    fct_ptr(arg);
    user_specified_timer = (mysecond() - start);

    printf("\n======== Kernel execution complete ========\n\n");
}