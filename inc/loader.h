#include "platform.h"

struct model_;
struct camera_;

typedef enum loader_state_ {
    LOADER_START,
    LOADER_TRIANGLE_COUNT,
    LOADER_RAM_BUFFER,
    LOADER_GPU_BUFFER,
    LOADER_WORKER_GPU,
    LOADER_DONE,

    LOADER_ERROR, /* Lower bound for error codes */
    LOADER_ERROR_NO_FILE,
    LOADER_ERROR_BAD_ASCII_STL,
    LOADER_ERROR_WRONG_SIZE,
} loader_state_t;

typedef struct loader_ {
    const char* filename;

    /*  Model parameters */
    GLuint vbo;
    uint32_t num_triangles;
    float mat[4][4];

    /*  GPU-mapped buffer, populated by main thread */
    float* buffer;

    /*  Synchronization system */
    platform_thread_t thread;
    loader_state_t state;
    platform_mutex_t mutex;
    platform_cond_t cond;

} loader_t;

loader_t* loader_new(const char* filename);
void loader_delete(loader_t* loader);

void loader_wait(loader_t* loader, loader_state_t target);
void loader_next(loader_t* loader, loader_state_t target);

void loader_allocate_vbo(loader_t* loader);
void loader_finish(loader_t* loader, struct model_* model,
                   struct camera_* camera);

const char* loader_error_string(loader_state_t state);
