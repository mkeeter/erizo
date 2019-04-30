#include "base.h"

struct theme_;

typedef struct backdrop_ {
    GLuint vao;
    GLuint vbo;

    /*  Shader program */
    GLuint vs;
    GLuint fs;
    GLuint prog;

    /*  Uniform locations */
    GLint u_upper_left;
    GLint u_upper_right;
    GLint u_lower_left;
    GLint u_lower_right;
} backdrop_t;

backdrop_t* backdrop_new();
void backdrop_delete(backdrop_t* backdrop);
void backdrop_draw(backdrop_t* backdrop, struct theme_* theme);
