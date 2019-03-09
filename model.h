#ifndef MODEL_H
#define MODEL_H
#include <stdint.h>

typedef struct model_ {
    uint32_t num_triangles;
    float mat[16];
} model_t;

#endif
