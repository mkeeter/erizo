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

typedef struct loader_ loader_t;

loader_t* loader_new(const char* filename);
void loader_delete(loader_t* loader);

void loader_wait(loader_t* loader, loader_state_t target);
void loader_next(loader_t* loader, loader_state_t target);

void loader_allocate_vbo(loader_t* loader);
void loader_finish(loader_t* loader, struct model_* model,
                   struct camera_* camera);

/*  Returns an error string based on loader->state, or NULL
 *  if the state is LOADER_DONE. */
const char* loader_error_string(loader_t* loader);

/*  Increments the mutex-protected count variable, notifying anything
 *  that is waiting on its condition variable. */
void loader_increment_count(loader_t* loader);
