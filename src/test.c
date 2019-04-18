#include "vset.h"
#include "platform.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage:  erizo-test model.stl\n");
        return 1;
    }

    size_t size;
    const char* data = platform_mmap(argv[1], &size);

    const int64_t start_time = platform_get_time();
    uint32_t tri_count;
    memcpy(&tri_count, &data[80], sizeof(tri_count));

    vset_t* v = vset_with_capacity(tri_count * 3);
    for (unsigned i=0; i < tri_count; ++i) {
        float vert3[9];
        memcpy(vert3, &data[84 + 12 + i*50], sizeof(vert3));
        for (unsigned j=0; j < 3; ++j) {
            vset_insert(v, &vert3[j * 3]);
        }
    }
    const int64_t dt = platform_get_time() - start_time;
    printf("Got %u triangles and %lu unique vertices\n", tri_count, v->count);
    printf("Ran in %lli.%06lli\n", dt / 1000000, dt % 1000000);

    platform_munmap(data, size);
}

