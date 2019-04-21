#include "base.h"

typedef struct ao_ {
    GLuint fbo;
    GLuint depth;
    GLuint tex;
    GLuint vol[2];
} ao_t;

ao_t* ao_new();
void ao_delete(ao_t* ao);
