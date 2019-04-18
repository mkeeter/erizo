#include "base.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

GLuint shader_build(const GLchar* src, GLenum type);
GLuint shader_link_vgf(GLuint vs, GLuint gs, GLuint fs);
GLuint shader_link_vf(GLuint vs, GLuint fs);

#define SHADER_GET_UNIFORM_LOC(object, target) \
    object->u_##target = glGetUniformLocation(object->prog, #target)
