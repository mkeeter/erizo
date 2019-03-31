#include "platform.h"

struct loader_;

typedef struct worker_ {
    platform_thread_t thread;
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

    /*  Bounds for this set of vertices */
    float min[3];
    float max[3];
} worker_t;

void worker_start(worker_t* worker);
