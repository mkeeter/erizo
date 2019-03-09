#include "loader.h"
#include "worker.h"

void* worker_run(void* worker_) {
    worker_t* const worker = (worker_t*)worker_;
    loader_t* const loader = worker->loader;

    loader_wait(loader, LOADER_GOT_BUFFER);

    /*  Copy data to the GPU buffer */
    size_t index = 80 + 4 + 3 * sizeof(float);
    const size_t index_stride = 12 * sizeof(float) + sizeof(uint16_t);
    index += index_stride * worker->start;
    for (uint32_t t=worker->start; t < worker->end; t += 1) {
        memcpy(&loader->buffer[t * 9], &loader->mapped[index], 9 * sizeof(float));
        index += index_stride;
    }

    return NULL;
}

