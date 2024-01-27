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

    return 0;
}