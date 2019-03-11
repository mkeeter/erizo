#ifndef BACKDROP_H
#define BACKDROP_H

#include "platform.h"

typedef struct backdrop_ {
    GLuint vao;
    GLuint vbo;
    GLuint prog;
} backdrop_t;

void backdrop_init(backdrop_t* backdrop);
void backdrop_draw(backdrop_t* backdrop);

#endif
