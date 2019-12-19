#include "log.h"
#include "shader.h"

static GLuint shader_build(const GLchar* src, GLenum type) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        GLchar* buf = (GLchar*)malloc(len + 1);
        glGetShaderInfoLog(shader, len, NULL, buf);
        log_error_and_abort("Failed to build shader: %s", buf);
        free(buf);
    }

    return shader;
}

shader_t shader_new(const char* vs, const char* gs, const char* fs) {
    shader_t shader;

    shader.prog = glCreateProgram();

    shader.vs = shader_build(vs, GL_VERTEX_SHADER);
    glAttachShader(shader.prog, shader.vs);

    shader.fs = shader_build(fs, GL_FRAGMENT_SHADER);
    glAttachShader(shader.prog, shader.fs);

    if (gs) {
        shader.gs = shader_build(gs, GL_GEOMETRY_SHADER);
        glAttachShader(shader.prog, shader.gs);
    }

    glLinkProgram(shader.prog);
    GLint status;
    glGetProgramiv(shader.prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint len = 0;
        glGetProgramiv(shader.prog, GL_INFO_LOG_LENGTH, &len);

        GLchar* buf = (GLchar*)malloc(len + 1);
        glGetProgramInfoLog(shader.prog, len, NULL, buf);
        log_error_and_abort("Failed to link program: %s", buf);
    }
    return shader;
}

void shader_deinit(shader_t shader) {
    glDeleteShader(shader.vs);
    glDeleteShader(shader.gs);
    glDeleteShader(shader.fs);
    glDeleteProgram(shader.prog);
}
