#ifndef LOADER_H
#define LOADER_H

#include "platform.h"

struct model_;

typedef enum loader_state_ {
    LOADER_START,
    LOADER_GOT_SIZE,
    LOADER_GOT_BUFFER,
} loader_state_t;

typedef struct loader_ {
    const char* filename;
    const char* mapped; /* mmapped file */
    float* buffer;      /* Mapped by OpenGL */

    /*  Synchronization system with the main thread */
    loader_state_t state;
    platform_mutex_t mutex;
    platform_cond_t cond;

    struct model_* model;
} loader_t;

void loader_wait(loader_t* loader, loader_state_t target);
void loader_next(loader_t* loader, loader_state_t target);
void* loader_run(void* loader_);

#endif
