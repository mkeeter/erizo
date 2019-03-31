#include "loader.h"
#include "log.h"
#include "worker.h"
#include "vset.h"

static void* worker_run(void* worker_);

void worker_start(worker_t* worker) {
    if (platform_thread_create(&worker->thread, worker_run, worker)) {
        log_error_and_abort("Error creating worker thread");
    }
}

static void* worker_run(void* worker_) {
    worker_t* const worker = (worker_t*)worker_;
    loader_t* const loader = worker->loader;

    /*  Build the deduplicated set of indexed vertices + triangles */
    vset_t* vset = vset_with_capacity(worker->tri_count * 3);
    uint32_t* tris = (uint32_t*)malloc(sizeof(uint32_t) * 3 * worker->tri_count);
    for (size_t i=0; i < worker->tri_count; ++i) {
        vset_insert_tri(vset, (const char*)&worker->stl[i], &tris[i * 3]);
    }

    worker->vert_count = vset->count;

    /*  Increment the number of finished worker threads */
    platform_mutex_lock(&loader->mutex);
    loader->count++;
    platform_cond_broadcast(&loader->cond);
    platform_mutex_unlock(&loader->mutex);

    /*  Find our model's bounds by iterating over deduplicated vertices */
    memcpy(worker->min, worker->stl, sizeof(worker->min));
    memcpy(worker->max, worker->stl, sizeof(worker->max));
    for (size_t i=0; i < vset->count; ++i) {
        for (unsigned j=0; j < 3; ++j) {
            const float v = vset->data[i][j];
            if (v < worker->min[j]) {
                worker->min[j] = v;
            }
            if (v > worker->max[j]) {
                worker->max[j] = v;
            }
        }
    }

    /*  Wait for GPU buffer pointers to be assigned */
    loader_wait(loader, LOADER_WORKER_GPU);

    /*  Send the vertex data to the GPU buffer */
    memcpy(worker->vertex_buf, &vset->data[1], 3 * sizeof(float) * vset->count);

    /*  Send the indexed triangles to the GPU buffer */
    memcpy(worker->index_buf, tris, 3 * sizeof(uint32_t) * worker->tri_count);

    free(tris);
    vset_delete(vset);

    return NULL;
}
