#include "base.h"

struct model_;
struct camera_;

typedef enum loader_state_ {
    LOADER_START,

    /*  The loader has populated the triangle and vertex counts, so the
     *  OpenGL thread can allocate and map buffers */
    LOADER_MODEL_SIZE,

    /*  The OpenGL thread has allocated and mapped buffers */
    LOADER_GPU_BUFFER,

    /*  The loader thread has split the GPU buffer to each worker */
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
    GLuint ibo;
    uint32_t tri_count;
    uint32_t vert_count;
    float mat[4][4];

    /*  GPU-mapped buffers, populated by main thread */
    float* vertex_buf;
    uint32_t* index_buf;

    /*  Synchronization system */
    struct platform_thread_* thread;
    loader_state_t state;
    struct platform_mutex_* mutex;
    struct platform_cond_* cond;

    /*  Each worker thread increments count when they are
     *  done building their vertex set, using the same mutex
     *  and condition variable. */
    unsigned count;

} loader_t;

loader_t* loader_new(const char* filename);
void loader_delete(loader_t* loader);

void loader_wait(loader_t* loader, loader_state_t target);
void loader_next(loader_t* loader, loader_state_t target);

void loader_allocate_vbo(loader_t* loader);
void loader_finish(loader_t* loader, struct model_* model,
                   struct camera_* camera);

const char* loader_error_string(loader_state_t state);
