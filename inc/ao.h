#include "base.h"

struct model_;
struct camera_;

typedef struct ao_depth_ {
    GLuint fbo;
    GLuint rbo;
    GLuint tex;

    GLuint vs;
    GLuint fs;
    GLuint prog;
    GLuint u_model;

    unsigned size;
} ao_depth_t;

typedef struct ao_vol_ {
    GLuint fbo;
    GLuint rbo;
    GLuint tex[2];
    unsigned logsize;

    GLuint vao;
    GLuint vbo;

    GLuint vs;
    GLuint fs;
    GLuint prog;
} ao_vol_t;

typedef struct ao_ {
    ao_depth_t depth;
    ao_vol_t vol;
} ao_t;

ao_t* ao_new();
void ao_delete(ao_t* ao);

void ao_render(ao_t* ao, struct model_* model, struct camera_* camera);
