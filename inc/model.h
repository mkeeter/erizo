#include "base.h"

typedef struct model_ {
    uint32_t tri_count;

    GLuint vao;
    GLuint vbo;
    GLuint ibo;
} model_t;

model_t* model_new();
void model_delete(model_t* model);
