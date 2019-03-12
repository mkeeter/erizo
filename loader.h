#include "platform.h"

struct model_;

typedef enum loader_state_ {
    LOADER_IDLE,
    LOADER_START,
    LOADER_TRIANGLE_COUNT,
    LOADER_RAM_BUFFER,
    LOADER_GPU_BUFFER,
    LOADER_WORKER_GPU,
    LOADER_DONE,
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
    platform_thread_t thread;
    loader_state_t state;
    platform_mutex_t mutex;
    platform_cond_t cond;

} loader_t;

void loader_init(loader_t* loader);
void loader_start(loader_t* loader, const char* filename);
loader_state_t loader_state(loader_t* loader);

void loader_wait(loader_t* loader, loader_state_t target);
void loader_next(loader_t* loader, loader_state_t target);

void loader_allocate_vbo(loader_t* loader);
void loader_finish(loader_t* loader, struct model_ * model);
void loader_reset(loader_t* loader);
