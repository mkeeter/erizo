#include "base.h"

struct model_;
struct camera_;

typedef struct ao_ {
    GLuint fbo;
    GLuint depth;

    GLuint depth_tex;     // Render texture
    GLuint vol_tex[2];  // AO textures

    unsigned depth_size;
    unsigned vol_logsize;

    GLuint vol_vao; // VAO for AO texture rendering
    GLuint vol_vbo; // VBO for AO texture rendering

    GLuint depth_vs;
    GLuint depth_fs;
    GLuint depth_prog;
    GLuint u_model;
} ao_t;

ao_t* ao_new();
void ao_delete(ao_t* ao);

void ao_render(ao_t* ao, struct model_* model, struct camera_* camera);
