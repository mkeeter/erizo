#include "loader.h"
#include "log.h"
#include "platform.h"
#include "worker.h"
#include "vset.h"

static void* worker_run(void* worker_);

void worker_start(worker_t* worker) {
    worker->thread = platform_thread_new(worker_run, worker);
}

void worker_finish(worker_t* worker) {
    if (platform_thread_join(worker->thread)) {
        log_error_and_abort("Error joining worker thread");
    }
    platform_thread_delete(worker->thread);
}

static void* worker_run(void* worker_) {
    worker_t* const worker = (worker_t*)worker_;
    loader_t* const loader = worker->loader;

    /*  Prepare to build the deduplicated set of indexed verts + tris */
    vset_t* vset = vset_new();
    uint32_t* tris = (uint32_t*)malloc(sizeof(uint32_t) * 3 * worker->tri_count);

    /*  Each triangle in an STL is 36 float-bytes (representing 3 vertices
     *  of 3 floats each), and they are spaced at 50-byte intervals.
     *
     *  This means that every other set of float-bytes is aligned, which
     *  lets us save a memcpy.  We check alignment and handle the unaligned
     *  case below, which then lets us loop faster. */
    unsigned i = 0;
#define INSERT_UNALIGNED() {                                    \
        float vert3[9];                                         \
        memcpy(vert3, &worker->stl[i], sizeof(vert3));          \
        for (unsigned j=0; j < 3; ++j) {                        \
            tris[i*3 + j] = vset_insert(vset, &vert3[j * 3]);   \
        }                                                       \
        i++;                                                    \
    }

    if ((uintptr_t)worker->stl & 3) {
        INSERT_UNALIGNED();
    }
    while (i < worker->tri_count) {
        for (unsigned j=0; j < 3; ++j) {
            tris[i*3 + j] = vset_insert(vset, (float*)worker->stl[i] + 3*j);
        }
        if (++i < worker->tri_count) {
            INSERT_UNALIGNED();
        }
    }
    worker->vert_count = vset->count;

    /*  Increment the number of finished worker threads */
    loader_increment_count(loader);

    /*  Find our model's bounds by iterating over deduplicated vertices */
    memcpy(worker->min, vset->vert[1], sizeof(worker->min));
    memcpy(worker->max, vset->vert[1], sizeof(worker->max));
    for (size_t i=1; i <= vset->count; ++i) {
        for (unsigned j=0; j < 3; ++j) {
            const float v = vset->vert[i][j];
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
    memcpy(worker->vertex_buf, vset->vert[1], 3 * sizeof(float) * vset->count);

    /*  Send the indexed triangles to the GPU buffer */
    memcpy(worker->index_buf, tris, 3 * sizeof(uint32_t) * worker->tri_count);

    free(tris);
    vset_delete(vset);

    return NULL;
}
