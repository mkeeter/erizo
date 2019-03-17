#include "platform.h"

typedef struct backdrop_ {
    GLuint vao;
    GLuint vbo;
    GLuint prog;

    GLuint u_gradient;
} backdrop_t;

backdrop_t* backdrop_new();
void backdrop_draw(backdrop_t* backdrop);
