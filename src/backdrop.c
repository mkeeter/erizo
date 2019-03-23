#include "backdrop.h"
#include "color.h"
#include "log.h"
#include "object.h"
#include "shader.h"

const GLchar* BACKDROP_VS_SRC = GLSL(330,
layout(location=0) in vec2 pos;

uniform vec3 upper_left;
uniform vec3 upper_right;
uniform vec3 lower_left;
uniform vec3 lower_right;

out vec4 grad_color;

void main() {
    if (pos.x < 0.0f && pos.y < 0.0f) {
        grad_color = vec4(lower_left, 1.0f);
    } else if (pos.x > 0.0f && pos.y < 0.0f) {
        grad_color = vec4(lower_right, 1.0f);
    } else if (pos.x < 0.0f && pos.y > 0.0f) {
        grad_color = vec4(upper_left, 1.0f);
    } else if (pos.x > 0.0f && pos.y > 0.0f) {
        grad_color = vec4(upper_right, 1.0f);
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
    backdrop->vs = shader_build(BACKDROP_VS_SRC, GL_VERTEX_SHADER);
    backdrop->fs = shader_build(BACKDROP_FS_SRC, GL_FRAGMENT_SHADER);
    backdrop->prog = shader_link_vf(backdrop->vs, backdrop->fs);
    glUseProgram(backdrop->prog);

    GET_UNIFORM_LOC(backdrop, upper_left);
    GET_UNIFORM_LOC(backdrop, upper_right);
    GET_UNIFORM_LOC(backdrop, lower_left);
    GET_UNIFORM_LOC(backdrop, lower_right);

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

void backdrop_delete(backdrop_t* backdrop) {
    glDeleteBuffers(1, &backdrop->vbo);
    glDeleteVertexArrays(1, &backdrop->vao);
    glDeleteShader(backdrop->vs);
    glDeleteShader(backdrop->fs);
    glDeleteProgram(backdrop->prog);
    free(backdrop);
}

void backdrop_draw(backdrop_t* backdrop) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(backdrop->prog);
    color_from_hex("003440", backdrop->u_upper_left);
    color_from_hex("002833", backdrop->u_upper_right);
    color_from_hex("002833", backdrop->u_lower_left);
    color_from_hex("002833", backdrop->u_lower_right);
    glBindVertexArray(backdrop->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
