//Our own implementation of Argobots runtime with custom work stealing
//scheduler for User level thread (ULTs).

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <abt.h>
// #include "abt.h"

typedef struct unit_t unit_t;
typedef struct pool_t pool_t;

struct unit_t {
    unit_t *p_prev;
    unit_t *p_next;
    ABT_thread thread;
};

struct pool_t {
    pthread_mutex_t lock;
    unit_t *p_head;
    unit_t *p_tail;
};

/* Pool functions */
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

static ABT_thread pool_pop(ABT_pool pool, ABT_pool_context context)
{
    pool_t *p_pool;
    ABT_pool_get_data(pool, (void **)&p_pool);
    unit_t *p_unit = NULL;
    pthread_mutex_lock(&p_pool->lock);
    if (p_pool->p_head == NULL) {
        /* Empty. */
    } else if (p_pool->p_head == p_pool->p_tail) {
        /* Only one thread. */
        p_unit = p_pool->p_head;
        p_pool->p_head = NULL;
        p_pool->p_tail = NULL;
    } else if (context & ABT_POOL_CONTEXT_OWNER_SECONDARY) {
        /* Pop from the tail. */
        p_unit = p_pool->p_tail;
        p_pool->p_tail = p_unit->p_next;
    } else {
        /* Pop from the head. */
        p_unit = p_pool->p_head;
        p_pool->p_head = p_unit->p_prev;
    }
    pthread_mutex_unlock(&p_pool->lock);
    if (!p_unit)
        return ABT_THREAD_NULL;
    return p_unit->thread;
}

static void pool_push(ABT_pool pool, ABT_unit unit, ABT_pool_context context)
{
    pool_t *p_pool;
    ABT_pool_get_data(pool, (void **)&p_pool);
    unit_t *p_unit = (unit_t *)unit;

    /* Lockless push to the pool. */

    if (p_pool->p_head) {
        p_unit->p_prev = p_pool->p_head;
        p_pool->p_head->p_next = p_unit;
    } else {
        p_pool->p_tail = p_unit;
    }
    p_pool->p_head = p_unit;
}

static int pool_init(ABT_pool pool, ABT_pool_config config)
{
    pool_t *p_pool = (pool_t *)calloc(1, sizeof(pool_t));
    if (!p_pool)
        return ABT_ERR_MEM;

    /* Initialize the spinlock */
    int ret = pthread_mutex_init(&p_pool->lock, 0);
    if (ret != 0) {
        free(p_pool);
        return ABT_ERR_SYS;
    }
    ABT_pool_set_data(pool, (void *)p_pool);
    return ABT_SUCCESS;
}

static void pool_free(ABT_pool pool)
{
    pool_t *p_pool;
    ABT_pool_get_data(pool, (void **)&p_pool);
    pthread_mutex_destroy(&p_pool->lock);
    free(p_pool);
}

static void create_pools(int num, ABT_pool *pools) //to be called in HClib::initialize()
{
    /* Pool definition */
    ABT_pool_user_def def;
    ABT_pool_user_def_create(pool_create_unit, pool_free_unit, pool_is_empty,
                             pool_pop, pool_push, &def);
    ABT_pool_user_def_set_init(def, pool_init);
    ABT_pool_user_def_set_free(def, pool_free);
    /* Pool configuration */
    ABT_pool_config config;
    ABT_pool_config_create(&config);
    /* The same as a pool created by ABT_pool_create_basic(). */
    const int automatic = 1;
    ABT_pool_config_set(config, ABT_pool_config_automatic.key,
                        ABT_pool_config_automatic.type, &automatic);

    int i;
    for (i = 0; i < num; i++) {
        ABT_pool_create(def, config, &pools[i]);
    }
    ABT_pool_user_def_free(&def);
    ABT_pool_config_free(&config);
}