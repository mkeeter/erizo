#include "backdrop.h"
#include "log.h"
#include "object.h"
#include "shader.h"

const GLchar* BACKDROP_VS_SRC = GLSL(330,
layout(location=0) in vec2 pos;

uniform mat4 gradient;
out vec4 grad_color;

void main() {
    if (pos.x < 0.0f && pos.y < 0.0f) {
        grad_color = gradient[0];
    } else if (pos.x > 0.0f && pos.y < 0.0f) {
        grad_color = gradient[1];
    } else if (pos.x < 0.0f && pos.y > 0.0f) {
        grad_color = gradient[2];
    } else if (pos.x > 0.0f && pos.y > 0.0f) {
        grad_color = gradient[3];
    }

    gl_Position = vec4(pos, 1.0f, 1.0f);
}
);

const GLchar* BACKDROP_FS_SRC = GLSL(330,
in vec4 grad_color;
out vec4 out_color;

void main() {
    out_color = grad_color;
}
);

backdrop_t* backdrop_new() {
    OBJECT_ALLOC(backdrop);
    GLuint vs = shader_build(BACKDROP_VS_SRC, GL_VERTEX_SHADER);
    GLuint fs = shader_build(BACKDROP_FS_SRC, GL_FRAGMENT_SHADER);
    backdrop->prog = shader_link_vf(vs, fs);
    glUseProgram(backdrop->prog);
    backdrop->u_gradient = glGetUniformLocation(backdrop->prog, "gradient");

    const float corners[] = {-1.0f, -1.0f,
                             -1.0f,  1.0f,
                              1.0f, -1.0f,
                              1.0f,  1.0f};
    glGenBuffers(1, &backdrop->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, backdrop->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);

    glGenVertexArrays(1, &backdrop->vao);
    glBindVertexArray(backdrop->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    log_trace("Initialized backdrop");

    return backdrop;
}

void backdrop_draw(backdrop_t* backdrop) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(backdrop->prog);
    const float gradient[] = {0.00f, 0.10f, 0.15f, 1.0f,
                              0.00f, 0.12f, 0.18f, 1.0f,
                              0.03f, 0.21f, 0.26f, 1.0f,
                              0.06f, 0.26f, 0.30f, 1.0f};
    glUniformMatrix4fv(backdrop->u_gradient, 1, GL_FALSE, gradient);
    glBindVertexArray(backdrop->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
