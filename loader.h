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

    GLuint vbo;
    uint32_t num_triangles;
    float mat[16];

    /*  Synchronization system with the main thread */
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
