#include "backdrop.h"
#include "log.h"
#include "shader.h"

const GLchar* BACKDROP_VS_SRC = GLSL(330,
layout(location=0) in vec2 pos;

void main() {
    gl_Position = vec4(pos, 1.0f, 1.0f);
}
);

const GLchar* BACKDROP_GS_SRC = GLSL(330,
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

void main() {
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}
);

const GLchar* BACKDROP_FS_SRC = GLSL(330,
out vec4 out_color;

void main() {
    out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
);

void backdrop_init(backdrop_t* backdrop) {
    GLuint vs = shader_build(BACKDROP_VS_SRC, GL_VERTEX_SHADER);
    GLuint gs = shader_build(BACKDROP_GS_SRC, GL_GEOMETRY_SHADER);
    GLuint fs = shader_build(BACKDROP_FS_SRC, GL_FRAGMENT_SHADER);
    backdrop->prog = shader_link(vs, gs, fs);

    const float corners[] = {-1.0f, -1.0f,
                              1.0f, -1.0f,
                             -1.0f,  1.0f,
                              1.0f,  1.0f};
    glGenBuffers(1, &backdrop->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, backdrop->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);

    glGenVertexArrays(1, &backdrop->vao);
    glBindVertexArray(backdrop->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    log_trace("Initialized backdrop");
}

void backdrop_draw(backdrop_t* backdrop) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(backdrop->prog);
    glBindVertexArray(backdrop->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
}
