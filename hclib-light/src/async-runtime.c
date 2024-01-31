#include "hclib-internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include "abt.h"

#define NUM_EXSTREAMS 4
#define NUM_ULTS 8

ABT_xstream exstreams[NUM_EXSTREAMS];
ABT_sched scheds[NUM_EXSTREAMS];
ABT_pool pools[NUM_EXSTREAMS];

void hclib_kernel(generic_frame_ptr fct_ptr, void * arg) {
    ABT_thread *threads;
    ABT_xstream xstream;
    int rank;

    ABT_xstream_self(&xstream);
    ABT_xstream_get_rank(xstream, &rank);

    double start = mysecond();
    ABT_thread_create(pools[rank], fct_ptr, (void *)arg, ABT_THREAD_ATTR_NULL, &threads);
    ABT_thread_free(&threads);

    user_specified_timer = (mysecond() - start)*1000;
    free(threads);
}

void hclib_finish(generic_frame_ptr fct_ptr, void * arg) {
    ABT_thread *threads;
    ABT_xstream xstream;
    int rank;

    ABT_xstream_self(&xstream);
    ABT_xstream_get_rank(xstream, &rank);

    ABT_thread_create(pools[rank], fct_ptr, (void *)arg, ABT_THREAD_ATTR_NULL, &threads);

    ABT_thread_free(&threads);
    free(threads);
}

void hclib_async(generic_frame_ptr fct_ptr, void * arg){
    ABT_thread *threads;
    ABT_xstream xstream;
    int rank;

    ABT_xstream_self(&xstream);
    ABT_xstream_get_rank(xstream, &rank);

    ABT_thread_create(pools[rank], fct_ptr, (void *)arg, ABT_THREAD_ATTR_NULL, &threads);

    ABT_thread_free(&threads);
    free(threads);
}