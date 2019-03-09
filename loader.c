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
    model_t* const m = loader->model;

    loader->mapped = platform_mmap(loader->filename);
    memcpy(&m->num_triangles, &loader->mapped[80], sizeof(m->num_triangles));
    loader_next(loader, LOADER_GOT_SIZE);

    /*  We kick off our mmap-to-OpenGL copying workers here, even though the
     *  buffer won't be ready for a little while.  This means that as soon
     *  as the buffer is ready, they'll start! */
    const size_t NUM_WORKERS = 8;
    worker_t workers[NUM_WORKERS];
    platform_thread_t worker_threads[NUM_WORKERS];
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        workers[i].loader = loader;
        workers[i].start = i * m->num_triangles / NUM_WORKERS;
        workers[i].end = (i + 1) * m->num_triangles / NUM_WORKERS;
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

    for (uint32_t t=0; t < m->num_triangles; t += 1) {
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

    memset(m->mat, 0, sizeof(m->mat));
    m->mat[0] = 1.0f / scale;
    m->mat[5] = 1.0f / scale;
    m->mat[10] = 1.0f / scale;
    m->mat[15] = 1.0f;

    log_trace("Waiting for buffer...");
    loader_wait(loader, LOADER_GOT_BUFFER);
    log_trace("Got buffer in loader thread");

    for (i = 0; i < NUM_WORKERS; ++i) {
        if (platform_thread_join(&worker_threads[i])) {
            log_error_and_abort("Error joining worker thread");
        }
    }

    log_trace("Loader thread done");
    return NULL;
}

