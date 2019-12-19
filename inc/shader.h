#include "base.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

typedef struct shader_ {
    GLuint vs;
    GLuint gs;
    GLuint fs;
    GLuint prog;
} shader_t;

shader_t shader_new(const char* vs, const char* gs, const char* fs);
void shader_deinit(shader_t shader);

#define SHADER_GET_UNIFORM(target) do {                 \
    u.target = glGetUniformLocation(prog, #target);     \
    if (u.target == -1) {                               \
        log_error("Failed to get uniform " #target);    \
    }                                                   \
} while(0)
