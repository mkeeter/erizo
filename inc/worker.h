#include "base.h"

typedef struct worker_ {
    struct platform_thread_* thread;
    struct loader_* loader;

    /*  Mesh input */
    const char (*stl)[50];

    /*  Mapped OpenGL vertex and index buffer */
    float *vertex_buf;
    uint32_t *index_buf;

    /*  Number of triangles to process */
    size_t tri_count;

    /*  Calculated vertex count (after deduplication) */
    size_t vert_count;

    /*  Offset applied to triangle indices before loading them into the GPU
     *  buffer, since each worker starts numbering at 0 */
    int32_t tri_offset;

    /*  Bounds for this set of vertices */
    float min[3];
    float max[3];
} worker_t;

void worker_start(worker_t* worker);
void worker_finish(worker_t* worker);
