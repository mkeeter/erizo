#ifndef LOADER_H
#define LOADER_H

#include "platform.h"

struct model_;

typedef enum loader_state_ {
    LOADER_START,
    LOADER_TRIANGLE_COUNT,
    LOADER_RAM_BUFFER,
    LOADER_GPU_BUFFER,
    LOADER_WORKER_GPU,
} loader_state_t;

typedef struct loader_ {
    const char* filename;

    /*  Model parameters */
    GLuint vbo;
    uint32_t num_triangles;
    float mat[16];

    /*  GPU-mapped buffer, populated by main thread */
    float* buffer;

    /*  Synchronization system */
    loader_state_t state;
    platform_mutex_t mutex;
    platform_cond_t cond;

} loader_t;

void loader_wait(loader_t* loader, loader_state_t target);
void loader_next(loader_t* loader, loader_state_t target);
void* loader_run(void* loader_);

void loader_allocate_vbo(loader_t* loader);
void loader_finish(loader_t* loader, struct model_ * model);

#endif
