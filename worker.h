#ifndef WORKER_H
#define WORKER_H

#include "platform.h"

struct loader_;

typedef struct worker_ {
    struct loader_* loader;
    uint32_t start;
    uint32_t end;
} worker_t;

void* worker_run(void* worker_);

#endif
