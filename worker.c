#include "loader.h"
#include "log.h"
#include "worker.h"

void* worker_run(void* worker_) {
    worker_t* const worker = (worker_t*)worker_;
    loader_t* const loader = worker->loader;

    loader_wait(loader, LOADER_RAM_BUFFER);

    memcpy(worker->min, &worker->stl[0], sizeof(worker->min));
    memcpy(worker->min, &worker->stl[0], sizeof(worker->max));

    /*  Copy from the memmapped STL to flat RAM, finding bounds */
    for (size_t i=0; i < worker->count; ++i) {
        memcpy(&worker->ram[i], &worker->stl[i], 36);
        for (unsigned t=0; t < 3; ++t) {
            for (unsigned a=0; a < 3; ++a) {
                const float v = worker->ram[i][3 * t + a];
                if (v < worker->min[a]) {
                    worker->min[a] = v;
                }
                if (v > worker->max[a]) {
                    worker->max[a] = v;
                }
            }
        }
    }

    /*  Wait for GPU buffer pointers to be assigned, then deploy data */
    loader_wait(loader, LOADER_WORKER_GPU);
    for (i=0; i < worker->count; ++i) {
        memcpy(&worker->gpu[i], &worker->ram[i], 36);
    }

    return NULL;
}

