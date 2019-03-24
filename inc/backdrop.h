#include "platform.h"

struct theme_;

typedef struct backdrop_ {
    GLuint vao;
    GLuint vbo;

    /*  Shader program */
    GLuint vs;
    GLuint fs;
    GLuint prog;

    /*  Uniform locations */
    GLuint u_upper_left;
    GLuint u_upper_right;
    GLuint u_lower_left;
    GLuint u_lower_right;
} backdrop_t;

backdrop_t* backdrop_new();
void backdrop_delete(backdrop_t* backdrop);
void backdrop_draw(backdrop_t* backdrop, struct theme_* theme);
