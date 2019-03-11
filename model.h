#ifndef MODEL_H
#define MODEL_H

#include "platform.h"

typedef struct model_ {
    uint32_t num_triangles;
    float mat[16];

    GLuint vao;
    GLuint vbo;

    GLuint prog;
    GLuint u_proj;
    GLuint u_model;
} model_t;

void model_init(model_t* model);
void model_draw(model_t* model, const float* proj);

#endif
