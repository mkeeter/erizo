#include "platform.h"

struct camera_;
struct theme_;

typedef struct model_ {
    uint32_t num_triangles;

    GLuint vao;
    GLuint vbo;

    /*  Shader program */
    GLuint vs;
    GLuint gs;
    GLuint fs;
    GLuint prog;

    /*  Uniform locations for matrices */
    GLuint u_proj;
    GLuint u_view;
    GLuint u_model;

    /*  Uniform locations for lighting */
    GLuint u_key;
    GLuint u_fill;
    GLuint u_base;
} model_t;

model_t* model_new();
void model_delete(model_t* model);
void model_draw(model_t* model, struct camera_* camera, struct theme_* theme);
