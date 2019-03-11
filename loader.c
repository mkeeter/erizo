#include "loader.h"
#include "log.h"
#include "model.h"
#include "worker.h"

void loader_wait(loader_t* loader, loader_state_t target) {
    while (loader->state != target) {
        platform_mutex_lock(&loader->mutex);
        platform_cond_wait(&loader->cond, &loader->mutex);
        platform_mutex_unlock(&loader->mutex);
    }
}

void loader_next(loader_t* loader, loader_state_t target) {
    platform_mutex_lock(&loader->mutex);
    loader->state = target;
    platform_cond_broadcast(&loader->cond);
    platform_mutex_unlock(&loader->mutex);
}

void* loader_run(void* loader_) {
    loader_t* const loader = (loader_t*)loader_;

    size_t size;
    loader->mapped = platform_mmap(loader->filename, &size);
    memcpy(&loader->num_triangles, &loader->mapped[80],
           sizeof(loader->num_triangles));
    loader_next(loader, LOADER_GOT_SIZE);

    /*  We kick off our mmap-to-OpenGL copying workers here, even though the
     *  buffer won't be ready for a little while.  This means that as soon
     *  as the buffer is ready, they'll start! */
    const size_t NUM_WORKERS = 8;
    worker_t workers[NUM_WORKERS];
    platform_thread_t worker_threads[NUM_WORKERS];
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        workers[i].loader = loader;
        workers[i].start = i * loader->num_triangles / NUM_WORKERS;
        workers[i].end = (i + 1) * loader->num_triangles / NUM_WORKERS;
        if (platform_thread_create(&worker_threads[i], worker_run,
                                   &workers[i]))
        {
            log_error_and_abort("Error creating worker thread");
        }
    }

    size_t index = 80 + 4 + 3 * sizeof(float);
    float min[3];
    float max[3];
    memcpy(min, &loader->mapped[index], sizeof(min));
    memcpy(min, &loader->mapped[index], sizeof(max));

    for (uint32_t t=0; t < loader->num_triangles; t += 1) {
        float xyz[9];
        memcpy(xyz, &loader->mapped[index], 9 * sizeof(float));
        for (unsigned i=0; i < 3; ++i) {
            for (unsigned a=0; a < 3; ++a) {
                const float v = xyz[3 * i + a];
                if (v < min[a]) {
                    min[a] = v;
                }
                if (v > max[a]) {
                    max[a] = v;
                }
            }
        }
        index += 12 * sizeof(float) + sizeof(uint16_t);
    }

    float center[3];
    float scale = 0;
    for (i=0; i < 3; ++i) {
        center[i] = (min[i] + max[i]) / 2.0f;
        const float d = max[i] - min[i];
        if (d > scale) {
            scale = d;
        }
    }

    memset(loader->mat, 0, sizeof(loader->mat));
    loader->mat[0] = 1.0f / scale;
    loader->mat[12] = -center[0] / scale;
    loader->mat[5] = 1.0f / scale;
    loader->mat[13] = -center[1] / scale;
    loader->mat[10] = 1.0f / scale;
    loader->mat[14] = -center[2] / scale;
    loader->mat[15] = 1.0f;

    log_trace("Waiting for buffer...");
    loader_wait(loader, LOADER_GOT_BUFFER);
    log_trace("Got buffer in loader thread");

    for (i = 0; i < NUM_WORKERS; ++i) {
        if (platform_thread_join(&worker_threads[i])) {
            log_error_and_abort("Error joining worker thread");
        }
    }
    platform_munmap(loader->mapped, size);
    loader->mapped = NULL;

    log_trace("Loader thread done");
    return NULL;
}

void loader_allocate_vbo(loader_t* loader) {
    glGenBuffers(1, &loader->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, loader->vbo);
    loader_wait(loader, LOADER_GOT_SIZE);

    glBufferData(GL_ARRAY_BUFFER, loader->num_triangles * 36,
                 NULL, GL_STATIC_DRAW);
    loader->buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    loader_next(loader, LOADER_GOT_BUFFER);

    log_trace("Allocated buffer");
}

void loader_finish(loader_t* loader, model_t* model) {
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
    memcpy(model->mat, loader->mat, sizeof(model->mat));

    log_trace("Copied model from loader");
}

