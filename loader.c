#include "camera.h"
#include "loader.h"
#include "log.h"
#include "mat.h"
#include "model.h"
#include "worker.h"

static void* loader_run(void* loader_);

void loader_wait(loader_t* loader, loader_state_t target) {
    platform_mutex_lock(&loader->mutex);
    while (loader->state < target) {
        platform_cond_wait(&loader->cond, &loader->mutex);
    }
    platform_mutex_unlock(&loader->mutex);
}

void loader_next(loader_t* loader, loader_state_t target) {
    platform_mutex_lock(&loader->mutex);
    loader->state = target;
    platform_cond_broadcast(&loader->cond);
    platform_mutex_unlock(&loader->mutex);
}

void loader_init(loader_t* loader) {
    platform_mutex_init(&loader->mutex);
    platform_cond_init(&loader->cond);
    loader_next(loader, LOADER_IDLE);
}

void loader_start(loader_t* loader, const char* filename) {
    const loader_state_t s = loader_state(loader);
    if (s != LOADER_IDLE) {
        log_error_and_abort("Invalid loader state: %i", s);
    }

    loader->filename = filename;
    loader->buffer = NULL;
    loader_next(loader, LOADER_START);

    if (platform_thread_create(&loader->thread, loader_run, loader)) {
        log_error_and_abort("Error creating loader thread");
    }
}

static void* loader_run(void* loader_) {
    loader_t* loader = (loader_t*)loader_;

    size_t size;
    const char* mapped = platform_mmap(loader->filename, &size);
    memcpy(&loader->num_triangles, &mapped[80],
           sizeof(loader->num_triangles));
    loader_next(loader, LOADER_TRIANGLE_COUNT);

    float* ram = (float*)malloc(loader->num_triangles * 3 * 3 * sizeof(float));
    loader_next(loader, LOADER_RAM_BUFFER);

    /*  We kick off our mmap-to-OpenGL copying workers here, even though the
     *  buffer won't be ready for a little while.  This means that as soon
     *  as the buffer is ready, they'll start! */
    const size_t NUM_WORKERS = 8;
    worker_t workers[NUM_WORKERS];
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        const size_t start = i * loader->num_triangles / NUM_WORKERS;
        const size_t end = (i + 1) * loader->num_triangles / NUM_WORKERS;

        workers[i].loader = loader;
        workers[i].count = end - start;
        workers[i].stl = (const char (*)[50])&mapped[80 + 4 + 12 + 50 * start];
        workers[i].ram = (float (*)[9])&ram[start * 9];
        workers[i].gpu = NULL;

        worker_start(&workers[i]);
    }

    log_trace("Waiting for buffer...");
    loader_wait(loader, LOADER_GPU_BUFFER);

    /*  Populate GPU pointers, then kick off workers copying triangles */
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        const size_t start = i * loader->num_triangles / NUM_WORKERS;
        workers[i].gpu = (float (*)[9])&loader->buffer[start * 9];
    }
    loader_next(loader, LOADER_WORKER_GPU);
    log_trace("Sent buffers to worker threads");

    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        if (platform_thread_join(&workers[i].thread)) {
            log_error_and_abort("Error joining worker thread");
        }
    }
    log_trace("Joined worker threads");

    /*  Reduce min / max arrays from worker subprocesses */
    float center[3];
    float scale = 0;
    for (unsigned v=0; v < 3; ++v) {
        for (unsigned i=1; i < NUM_WORKERS; ++i) {
            if (workers[i].max[v] > workers[0].max[v]) {
                workers[0].max[v] = workers[i].max[v];
            }
            if (workers[i].min[v] < workers[0].min[v]) {
                workers[0].min[v] = workers[i].min[v];
            }
        }
        center[v] = (workers[0].max[v] + workers[0].min[v]) / 2.0f;
        const float d = workers[0].max[v] - workers[0].min[v];
        if (d > scale) {
            scale = d;
        }
    }
    /*  Translation component of model matrix */
    mat4_translation(center, loader->mat);

    /*  Scale component of model matrix */
    float scale_vec[3];
    for (unsigned s=0; s < 3; ++s) {
        scale_vec[s] = 1 / scale;
    }
    float scale_matrix[4][4];
    mat4_scaling(scale_vec, scale_matrix);
    mat4_mul(loader->mat, scale_matrix, loader->mat);

    /*  Mark the load as done and post an empty event, to make sure that
     *  the main loop wakes up and checks the loader */
    log_trace("Loader thread done");
    loader_next(loader, LOADER_DONE);
    glfwPostEmptyEvent();

    platform_munmap(mapped, size);
    free(ram);

    return NULL;
}

void loader_allocate_vbo(loader_t* loader) {
    glGenBuffers(1, &loader->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, loader->vbo);
    loader_wait(loader, LOADER_TRIANGLE_COUNT);

    glBufferData(GL_ARRAY_BUFFER, loader->num_triangles * 36,
                 NULL, GL_STATIC_DRAW);
    loader->buffer = glMapBufferRange(GL_ARRAY_BUFFER, 0, loader->num_triangles * 36,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
            GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    loader_next(loader, LOADER_GPU_BUFFER);

    log_trace("Allocated buffer");
}

loader_state_t loader_state(loader_t* loader) {
    platform_mutex_lock(&loader->mutex);
    loader_state_t out = loader->state;
    platform_mutex_unlock(&loader->mutex);
    return out;
}

void loader_finish(loader_t* loader, model_t* model, camera_t* camera) {
    if (!loader->vbo) {
        log_error_and_abort("Invalid loader VBO");
    } else if (!model->vao) {
        log_error_and_abort("Invalid model VAO");
    }

    glBindBuffer(GL_ARRAY_BUFFER, loader->vbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindVertexArray(model->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glDeleteBuffers(1, &model->vbo);
    model->vbo = loader->vbo;
    loader->vbo = 0;

    model->num_triangles = loader->num_triangles;
    memcpy(camera->model, loader->mat, sizeof(loader->mat));

    log_trace("Copied model from loader");
}

void loader_reset(loader_t* loader) {
    if (platform_thread_join(&loader->thread)) {
        log_error_and_abort("Failed to join loader thread");
    }
    loader_next(loader, LOADER_IDLE);
    log_trace("Reset loader");
}
