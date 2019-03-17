#include "platform.h"

typedef struct backdrop_ {
    GLuint vao;
    GLuint vbo;

    /*  Shader program */
    GLuint vs;
    GLuint fs;
    GLuint prog;

    /*  Uniform locations */
    GLuint u_gradient;
} backdrop_t;

backdrop_t* backdrop_new();
void backdrop_delete(backdrop_t* backdrop);
void backdrop_draw(backdrop_t* backdrop);
