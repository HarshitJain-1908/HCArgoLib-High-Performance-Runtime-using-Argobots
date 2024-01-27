//Our own implementation of Argobots runtime with custom work stealing
//scheduler for User level thread (ULTs).

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <abt.h>

#define DEFAULT_NUM_XSTREAMS 2
#define DEFAULT_NUM_THREADS 8

typedef struct {
    int tid;
} thread_arg_t;

void hello_world(void *arg)
{
    int tid = ((thread_arg_t *)arg)->tid;
    int rank;
    //ABT_xstream_self_rank(&rank);
    printf("Hello world! (thread = %d, ES = %d)\n", tid, rank);
}


int main(int argc, char **argv) {

    printf("Hello! HCArgoLib\n");
    printf("Number of ES = %d \n", DEFAULT_NUM_XSTREAMS);
    printf("Number of ULTs = %d \n", DEFAULT_NUM_THREADS);

    int i, j;
    /* Read arguments. */
    int num_xstreams = DEFAULT_NUM_XSTREAMS;
    int num_threads = DEFAULT_NUM_THREADS;
    
     /* Allocate memory. */
    ABT_xstream *xstreams =
        (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_xstreams);
    ABT_pool *pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_xstreams);
    ABT_sched *scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * num_xstreams);
    ABT_thread *threads =
        (ABT_thread *)malloc(sizeof(ABT_thread) * num_threads);
    thread_arg_t *thread_args =
        (thread_arg_t *)malloc(sizeof(thread_arg_t) * num_threads);

    /* Initialize Argobots. */
    ABT_init(argc, argv);

    /* Create pools. */
    for (i = 0; i < num_xstreams; i++) {
        ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE,
                              &pools[i]);
    }

    /* Create schedulers. */
    for (i = 0; i < num_xstreams; i++) {
        ABT_pool *tmp = (ABT_pool *)malloc(sizeof(ABT_pool) * num_xstreams);
        for (j = 0; j < num_xstreams; j++) {
            tmp[j] = pools[(i + j) % num_xstreams];
        }
        ABT_sched_create_basic(ABT_SCHED_DEFAULT, num_xstreams, tmp,
                               ABT_SCHED_CONFIG_NULL, &scheds[i]);
        free(tmp);
    }

    /* Set up a primary execution stream. */
    ABT_xstream_self(&xstreams[0]);
    ABT_xstream_set_main_sched(xstreams[0], scheds[0]);

    /* Create secondary execution streams. */
    for (i = 1; i < num_xstreams; i++) {
        ABT_xstream_create(scheds[i], &xstreams[i]);
    }

    /* Create ULTs. */
    for (i = 0; i < num_threads; i++) {
        int pool_id = i % num_xstreams;
        thread_args[i].tid = i;
        ABT_thread_create(pools[pool_id], hello_world, &thread_args[i],
                          ABT_THREAD_ATTR_NULL, &threads[i]);
    }

    /* Join and free ULTs. */
    for (i = 0; i < num_threads; i++) {
        ABT_thread_free(&threads[i]);
    }

    /* Join secondary execution streams. */
    for (i = 1; i < num_xstreams; i++) {
        ABT_xstream_join(xstreams[i]);
        ABT_xstream_free(&xstreams[i]);
    }

    /* Finalize Argobots. */
    ABT_finalize();

    /* Free allocated memory. */
    free(xstreams);
    free(pools);
    free(scheds);
    free(threads);
    free(thread_args);

    return 0;
}