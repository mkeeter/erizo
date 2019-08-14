#include "vset.h"
#include "eset.h"
#include "platform.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage:  erizo-test model.stl\n");
        return 1;
    }

    platform_mmap_t* map = platform_mmap(argv[1]);
    const char* data = platform_mmap_data(map);

    uint32_t tri_count;
    memcpy(&tri_count, &data[80], sizeof(tri_count));

    uint32_t (*buf)[6] = calloc(tri_count, sizeof(uint32_t) * 6);

    uint32_t vert_count;

#define WARM_UP 1
#define ITERATION_COUNT 5
    double dt_index[ITERATION_COUNT];
    double dt_edges[ITERATION_COUNT];
    for (unsigned i=0; i < WARM_UP + ITERATION_COUNT; ++i) {
        printf("\r%u / %u ", i + 1, WARM_UP + ITERATION_COUNT);
        fflush(stdout);

        int64_t start_time = platform_get_time();
        vset_t* v = vset_new();
        for (unsigned i=0; i < tri_count; ++i) {
            float vert3[9];
            memcpy(vert3, &data[84 + 12 + i*50], sizeof(vert3));
            for (unsigned j=0; j < 3; ++j) {
                buf[i][j] = vset_insert(v, &vert3[j * 3]);
            }
        }
        if (i >= WARM_UP) {
            dt_index[i - WARM_UP] = (platform_get_time() - start_time) / 1000000.0;
        }
        vert_count = v->count;
        vset_delete(v);

        start_time = platform_get_time();
        eset_t* e = eset_new();
        for (unsigned i=0; i < tri_count; ++i) {
            for (unsigned j = 0; j < 3; ++j) {
                const uint32_t a = buf[i][j];
                const uint32_t b = buf[i][(j + 1) % 3];
                if (a < b) {
                    eset_insert(e, (eset_edge_t){a, b}, i, j);
                }
            }
        }
        for (unsigned i=0; i < tri_count; ++i) {
            for (unsigned j = 0; j < 3; ++j) {
                const uint32_t a = buf[i][j];
                const uint32_t b = buf[i][(j + 1) % 3];
                if (b < a) {
                    int32_t t = eset_find(e, (eset_edge_t){b, a});
                    if (t != 0) {
                        buf[e->edge[t].tri][e->edge[t].sub + 3] = i;
                        buf[i][j + 3] = e->edge[t].tri;
                    }
                }
            }
        }
        if (i >= WARM_UP) {
            dt_edges[i - WARM_UP] = (platform_get_time() - start_time) / 1000000.0;
        }

        if (i == 0 && tri_count < 100) {
            printf("\n");
            for (unsigned t=0; t < tri_count; ++ t) {
                printf("%u %u %u\t%u %u %u\n", buf[t][0],
                    buf[t][1],
                    buf[t][2],
                    buf[t][3],
                    buf[t][4],
                    buf[t][5]);
            }
        }
    }
    platform_set_terminal_color(stdout, TERM_COLOR_WHITE);
    printf("\rvset performance test:\n");
    platform_clear_terminal_color(stdout);
    printf("    Triangles:          %u\n", tri_count);
    printf("    Unique vertices:    %u\n", vert_count);

    double mean_index = 0.0;
    for (unsigned i=0; i < ITERATION_COUNT; ++i) {
        mean_index += dt_index[i];
    }
    mean_index /= ITERATION_COUNT;

    double std_index = 0.0;
    for (unsigned i=0; i < ITERATION_COUNT; ++i) {
        std_index += pow(dt_index[i] - mean_index, 2.0);
    }
    std_index = sqrt(std_index / (ITERATION_COUNT - 1));

    double mean_edges = 0.0;
    for (unsigned i=0; i < ITERATION_COUNT; ++i) {
        mean_edges += dt_edges[i];
    }
    mean_edges /= ITERATION_COUNT;

    double std_edges = 0.0;
    for (unsigned i=0; i < ITERATION_COUNT; ++i) {
        std_edges += pow(dt_edges[i] - mean_edges, 2.0);
    }
    std_edges = sqrt(std_edges / (ITERATION_COUNT - 1));

    printf("    Time per iteration (indexing):    %f ± %f s\n", mean_index, std_index);
    printf("    Time per iteration (neighboring): %f ± %f s\n", mean_edges, std_edges);

    platform_munmap(map);
}
