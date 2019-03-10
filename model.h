#ifndef MODEL_H
#define MODEL_H

#include "platform.h"

typedef struct model_ {
    uint32_t num_triangles;
    float mat[16];

    /*  Specific to each model */
    GLuint vao;
    GLuint vbo;

    /*  Common to every model */
    GLuint prog;
} model_t;

GLuint model_build_shader();

#endif
