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

    /*  Prepare to build the deduplicated set of indexed verts + tris */
    vset_t* vset = vset_with_capacity(worker->tri_count * 3);
    uint32_t* tris = (uint32_t*)malloc(sizeof(uint32_t) * 3 * worker->tri_count);

    /*  This part is exciting, because STL files have 36 float-bytes
     *  (representing 3 vertices of 3 floats each) at 50-byte intervals.
     *
     *  This means that every other set of float-bytes is aligned, which
     *  lets us save a memcpy.  We check to see our parity, then iterate over
     *  fast (aligned) floats and unaligned bytes in two separate loops. */
    const bool unaligned = ((uintptr_t)worker->stl & 3);
    /*  First, we do the aligned loads */
    for (size_t i=unaligned; i < worker->tri_count; i += 2) {
        for (unsigned j=0; j < 3; ++j) {
            tris[i*3 + j] = vset_insert(vset, (float*)worker->stl[i] + 3*j);
        }
    }
    /*  Then, the unaligned loads (which require a memcpy) */
    for (size_t i=!unaligned; i < worker->tri_count; i += 2) {
        float vert3[9];
        memcpy(vert3, &worker->stl[i], sizeof(vert3));
        for (unsigned j=0; j < 3; ++j) {
            tris[i*3 + j] = vset_insert(vset, &vert3[j * 3]);
        }
    }
    worker->vert_count = vset->count;

    /*  Increment the number of finished worker threads */
    platform_mutex_lock(&loader->mutex);
    loader->count++;
    platform_cond_broadcast(&loader->cond);
    platform_mutex_unlock(&loader->mutex);

    /*  Find our model's bounds by iterating over deduplicated vertices */
    memcpy(worker->min, vset->data[1], sizeof(worker->min));
    memcpy(worker->max, vset->data[1], sizeof(worker->max));
    for (size_t i=1; i <= vset->count; ++i) {
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

    /*  Wait for the loader to set up our triangle offsets, so that
     *  each worker is referring to the correct part of the buffer. */
    loader_wait(loader, LOADER_MODEL_SIZE);
    for (unsigned i=0; i < worker->tri_count * 3; ++i) {
        tris[i] += worker->tri_offset - 1;
    }

    /*  Wait for GPU buffer pointers to be assigned */
    loader_wait(loader, LOADER_WORKER_GPU);

    /*  Send the vertex data to the GPU buffer */
    memcpy(worker->vertex_buf, vset->data[1], 3 * sizeof(float) * vset->count);

    /*  Send the indexed triangles to the GPU buffer */
    memcpy(worker->index_buf, tris, 3 * sizeof(uint32_t) * worker->tri_count);

    free(tris);
    vset_delete(vset);

    return NULL;
}
