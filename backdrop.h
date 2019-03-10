#ifndef BACKDROP_H
#define BACKDROP_H

#include "platform.h"

typedef struct view_ {
    GLuint vao;
    GLuint vbo;
    GLuint program;
} backdrop_t;

void backdrop_init(backdrop_t* backdrop);
void backdrop_draw(backdrop_t* backdrop);

#endif
