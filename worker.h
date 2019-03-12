#ifndef WORKER_H
#define WORKER_H

#include "platform.h"

struct loader_;

typedef struct worker_ {
    platform_thread_t thread;
    struct loader_* loader;

    /*  Inputs from loader */
    const char (*stl)[50];
    float (*ram)[9];
    float (*gpu)[9];
    size_t count;

    float min[3];
    float max[3];
} worker_t;

void worker_start(worker_t* worker);

#endif
