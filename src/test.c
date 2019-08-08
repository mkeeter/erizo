#include "vset.h"
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

    uint32_t vert_count;

#define WARM_UP 5
#define ITERATION_COUNT 20
    double dt[ITERATION_COUNT];
    for (unsigned i=0; i < WARM_UP + ITERATION_COUNT; ++i) {
        printf("\r%u / %u ", i + 1, WARM_UP + ITERATION_COUNT);
        fflush(stdout);

        const int64_t start_time = platform_get_time();
        vset_t* v = vset_new();
        for (unsigned i=0; i < tri_count; ++i) {
            float vert3[9];
            memcpy(vert3, &data[84 + 12 + i*50], sizeof(vert3));
            for (unsigned j=0; j < 3; ++j) {
                vset_insert(v, &vert3[j * 3]);
            }
        }
        if (i >= WARM_UP) {
            dt[i - WARM_UP] = (platform_get_time() - start_time) / 1000000.0;
        }
        vert_count = v->count;
        vset_delete(v);
    }
    platform_set_terminal_color(stdout, TERM_COLOR_WHITE);
    printf("\rvset performance test:\n");
    platform_clear_terminal_color(stdout);
    printf("    Triangles:          %u\n", tri_count);
    printf("    Unique vertices:    %u\n", vert_count);

    double mean = 0.0;
    for (unsigned i=0; i < ITERATION_COUNT; ++i) {
        mean += dt[i];
    }
    mean /= ITERATION_COUNT;

    double std = 0.0;
    for (unsigned i=0; i < ITERATION_COUNT; ++i) {
        std += pow(dt[i] - mean, 2.0);
    }
    std = sqrt(std / (ITERATION_COUNT - 1));

    printf("    Time per iteration: %f ± %f s\n", mean, std);

    platform_munmap(map);
}
