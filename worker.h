#ifndef WORKER_H
#define WORKER_H

#include "platform.h"

struct loader;

typedef struct {
    struct loader_* loader;
    uint32_t start;
    uint32_t end;
} worker_t;

void* worker_run(void* worker_);

#endif
