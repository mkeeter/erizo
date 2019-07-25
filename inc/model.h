#include "base.h"

struct camera_;
struct theme_;

typedef struct model_ {
    uint32_t tri_count;

    GLuint vao;
    GLuint vbo;
    GLuint ibo;

    /*  Shader program */
    GLuint vs;
    GLuint gs;
    GLuint fs;
    GLuint prog;

    /*  Uniform locations for matrices */
    GLint u_proj;
    GLint u_view;
    GLint u_model;

    /*  Uniform locations for lighting */
    GLint u_key;
    GLint u_fill;
    GLint u_base;

    /*  Uniform locations for volumetric lighting */
    GLint u_vol_tex;
    GLint u_vol_logsize;
    GLint u_vol_num_rays;
} model_t;

model_t* model_new();
void model_delete(model_t* model);
void model_draw(model_t* model, struct camera_* camera, struct theme_* theme);
