#include "base.h"

struct model_;
struct camera_;

typedef struct ao_ {
    GLuint fbo;
    GLuint depth;
    GLuint tex;
    GLuint vol[2];

    unsigned render_size;
    unsigned vol_logsize;

    GLuint vs;
    GLuint fs;
    GLuint prog;
    GLuint u_model;
} ao_t;

ao_t* ao_new();
void ao_delete(ao_t* ao);

void ao_render(ao_t* ao, struct model_* model, struct camera_* camera);
