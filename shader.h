#include "platform.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

GLuint shader_build(const GLchar* src, GLenum type);
GLuint shader_link(GLuint vs, GLuint gs, GLuint fs);
