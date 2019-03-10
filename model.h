#ifndef MODEL_H
#define MODEL_H

#include "platform.h"

typedef struct model_ {
    GLuint vao;
    GLuint vbo;

    uint32_t num_triangles;
    float mat[16];
} model_t;

#endif
