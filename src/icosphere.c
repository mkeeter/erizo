#include "icosphere.h"
#include "log.h"
#include "mat.h"
#include "vset.h"

struct icosphere_ {
    uint32_t num_vs;
    vec3_t* vs;

    uint32_t num_ts;
    uint32_t (*ts)[3];
};

uint32_t edge_lookup(icosphere_t* ico, uint32_t (*edge)[6][2],
                     const uint32_t ab[2]) {
    // Look to see whether this edge was already populated
    for (unsigned i=0; i < 6; ++i) {
        for (unsigned j=0; j < 2; ++j) {
            if (edge[ab[j]][i][0] == ab[!j]) {
                return edge[ab[j]][i][1];
            }
        }
    }

    // Otherwise, store the new vertex
    uint32_t n = ico->num_vs++;
    ico->vs[n] = vec3_normalized(vec3_center(ico->vs[ab[0]], ico->vs[ab[1]]));

    // Record the vertex index in the edge tables by finding the first
    // slot with zeros and replacing it with our new information.
    for (unsigned j=0; j < 2; ++j) {
        for (unsigned i=0; i < 6; ++i) {
            if (edge[ab[j]][i][0] == 0) {
                edge[ab[j]][i][0] = ab[!j];
                edge[ab[j]][i][1] = n;
                break;
            } else if (i == 5) {
                log_error_and_abort("Could not pack vertex");
            }
        }
    }

    return n;
}

icosphere_t* subdivide(const icosphere_t* in) {
    icosphere_t* ico = malloc(sizeof(icosphere_t));

    const uint32_t num_ts_next = in->num_ts * 4;
    const uint32_t num_vs_next = in->num_vs + in->num_ts * 3 / 2;

    ico->ts = malloc(num_ts_next * sizeof(*ico->ts));
    ico->vs = malloc(num_vs_next * sizeof(*ico->vs));
    ico->num_ts = 0;

    // Copy over the initial set of vertices, which will still exist
    // in the subdivided model
    memcpy(ico->vs, in->vs, in->num_vs * sizeof(*ico->vs));
    ico->num_vs = in->num_vs;

    /*  Every vertex has up to 6 neighbors.  This table records for each vertex
     *      a) The neighbor's index
     *      b) The index of the new vertex on that neighbor edge
     *  It is all zeros to start, because we have no neighbor information. */
    uint32_t (*edge)[6][2] = calloc(in->num_vs, sizeof(*edge));

    for (unsigned t=0; t < in->num_ts; ++t) {
        //  This is the triangle that we're currently operating on
        const uint32_t* tri = in->ts[t];

        /*  We subdivide each triangle into 4:
         *      0                  0
         *     / \                / \
         *    /   \    ==>       /---\
         *   /     \            / \ / \
         *  1-------2          1-------2
         */
        const uint32_t sub[4][3][2] = {
            {{0, 0}, {0, 1}, {0, 2}},
            {{0, 1}, {1, 1}, {1, 2}},
            {{0, 1}, {1, 2}, {2, 0}},
            {{1, 2}, {2, 2}, {2, 0}}};

        for (unsigned n=0; n < 4; ++n) {
            //  Find subtriangle indexes, which are either a previous
            //  vertex or a new vertex (stored in the edges array)
            const uint32_t t = ico->num_ts++;
            for (unsigned i=0; i < 3; ++i) {
                if (sub[n][i][0] == sub[n][i][1]) {
                    ico->ts[t][i] = tri[sub[n][i][0]];
                } else {
                    const uint32_t ab[] = {tri[sub[n][i][0]],
                                           tri[sub[n][i][1]]};
                    ico->ts[t][i] = edge_lookup(ico, edge, ab);
                }
            }
        }
    }
    if (ico->num_ts != num_ts_next) {
        log_error_and_abort("Wrong number of triangles (expected %u, got %u)",
                            num_ts_next, ico->num_ts);
    } else if (ico->num_vs != num_vs_next) {
        log_error_and_abort("Wrong number of vertices (expected %u, got %u)",
                            num_vs_next, ico->num_vs);
    }

    return ico;
}

icosphere_t* icosphere_new(unsigned depth) {
    icosphere_t* ico = malloc(sizeof(icosphere_t));

    {   // Load initial vertices into the vset
        const float t = (1.0f + sqrtf(5.0f)) / 2.0f;
        const float ts[13][3] = {
            { 0.0f, 0.0f, 0.0f}, // Dummy vertex

            {-1.0f,  t,  0.0f},
            { 1.0f,  t,  0.0f},
            {-1.0f, -t,  0.0f},
            { 1.0f, -t,  0.0f},

            { 0.0f, -1.0f,  t},
            { 0.0f,  1.0f,  t},
            { 0.0f, -1.0f, -t},
            { 0.0f,  1.0f, -t},

            { t,  0.0f, -1.0f},
            { t,  0.0f,  1.0f},
            {-t,  0.0f, -1.0f},
            {-t,  0.0f,  1.0f},
        };
        ico->num_vs = sizeof(ts) / sizeof(*ts);
        ico->vs = malloc(sizeof(ts));
        memcpy(ico->vs, ts, sizeof(ts));
        for (unsigned i=1; i < ico->num_vs; ++i) {
            ico->vs[i] = vec3_normalized(ico->vs[i]);
        }
    }

    {   // Create an array of triangles, accounting for the
        // dummy vertex at index zero.
        const uint32_t ts[20][3] = {
            {1, 12, 6},
            {1, 6, 2},
            {1, 2, 8},
            {1, 8, 11},
            {1, 11, 12},

            {2, 6, 10},
            {6, 12, 5},
            {12, 11, 3},
            {11, 8, 7},
            {8, 2, 9},

            {4, 10, 5},
            {4, 5, 3},
            {4, 3, 7},
            {4, 7, 9},
            {4, 9, 10},

            {5, 10, 6},
            {3, 5, 12},
            {7, 3, 11},
            {9, 7, 8},
            {10, 9, 2}};

        ico->num_ts = sizeof(ts) / sizeof(*ts);
        ico->ts = malloc(sizeof(ts));
        memcpy(ico->ts, ts, sizeof(ts));
    }


    for (unsigned d=0; d < depth; ++d) {
        icosphere_t* next = subdivide(ico);
        icosphere_delete(ico);
        ico = next;
    }
    return ico;
}

void icosphere_delete(icosphere_t* ico) {
    free(ico->vs);
    free(ico->ts);
    free(ico);
}

const char* icosphere_stl(unsigned depth, size_t* size) {
    icosphere_t* ico = icosphere_new(depth);
    *size = ico->num_ts * 50 + 84;
    char* out = calloc(1, *size);

    // Record the triangle count
    char* ptr = out + 80;
    memcpy(ptr, &ico->num_ts, 4);
    ptr += 4;

    // Copy over every triangle
    for (unsigned i=0; i < ico->num_ts; ++i) {
        ptr += 12;  // Skip normal vector
        for (unsigned j=0; j < 3; ++j) {
            memcpy(ptr, &ico->vs[ico->ts[i][j]], 12);
            ptr += 12;
        }
        ptr += 2;   // Skip attribute bytes
    }
    icosphere_delete(ico);
    return out;
}
