#include "platform.h"

struct camera_;

typedef struct model_ {
    uint32_t num_triangles;

    GLuint vao;
    GLuint vbo;

    GLuint prog;
    GLuint u_proj;
    GLuint u_view;
    GLuint u_model;
} model_t;

model_t* model_new();
void model_draw(model_t* model, struct camera_* camera);
