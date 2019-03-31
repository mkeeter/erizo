#include "camera.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "shader.h"
#include "theme.h"

const GLchar* MODEL_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

void main() {
    gl_Position = vec4(pos, 1.0f);
}
);

const GLchar* MODEL_GS_SRC = GLSL(330,
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec3 vert_norm;
out vec3 pos_bary;

void main() {
    vec3 a = gl_in[0].gl_Position.xyz;
    vec3 b = gl_in[1].gl_Position.xyz;
    vec3 c = gl_in[2].gl_Position.xyz;

    vec3 na = cross(a - b, c - b);
    vec3 nb = cross(b - c, a - c);
    vec3 nc = cross(c - a, b - a);
    vert_norm = normalize((view * model * vec4(na + nb + nc, 0.0f)).xyz);

    mat4 m = proj * view * model;

    pos_bary = vec3(1.0f, 0.0f, 0.0f);
    gl_Position = m * gl_in[0].gl_Position;
    EmitVertex();

    pos_bary = vec3(0.0f, 1.0f, 0.0f);
    gl_Position = m * gl_in[1].gl_Position;
    EmitVertex();

    pos_bary = vec3(0.0f, 0.0f, 1.0f);
    gl_Position = m * gl_in[2].gl_Position;
    EmitVertex();
}
);

const GLchar* MODEL_FS_SRC = GLSL(330,
in vec3 vert_norm;
in vec3 pos_bary;

uniform vec3 key;
uniform vec3 fill;
uniform vec3 base;

out vec4 out_color;

void main() {
    float a = dot(vert_norm, vec3(0.0f, 0.0f, 1.0f));
    float b = dot(vert_norm, vec3(-0.57f, -0.57f, 0.57f));

    out_color = vec4(mix(base, key,  a) * 0.5f +
                     mix(base, fill, b) * 0.5f, 1.0f);
}
);

model_t* model_new() {
    OBJECT_ALLOC(model);
    model->tri_count = 0;
    model->vbo = 0;
    model->ibo = 0;
    glGenVertexArrays(1, &model->vao);

    model->vs = shader_build(MODEL_VS_SRC, GL_VERTEX_SHADER);
    model->gs = shader_build(MODEL_GS_SRC, GL_GEOMETRY_SHADER);
    model->fs = shader_build(MODEL_FS_SRC, GL_FRAGMENT_SHADER);
    model->prog = shader_link_vgf(model->vs, model->gs, model->fs);

    SHADER_GET_UNIFORM_LOC(model, proj);
    SHADER_GET_UNIFORM_LOC(model, view);
    SHADER_GET_UNIFORM_LOC(model, model);

    SHADER_GET_UNIFORM_LOC(model, key);
    SHADER_GET_UNIFORM_LOC(model, fill);
    SHADER_GET_UNIFORM_LOC(model, base);

    log_trace("Initialized model");
    return model;
}

void model_delete(model_t* model) {
    glDeleteBuffers(1, &model->vbo);
    glDeleteVertexArrays(1, &model->vao);
    glDeleteShader(model->vs);
    glDeleteShader(model->gs);
    glDeleteShader(model->fs);
    glDeleteProgram(model->prog);
    free(model);
}

void model_draw(model_t* model, camera_t* camera, theme_t* theme) {
    glEnable(GL_DEPTH_TEST);
    glUseProgram(model->prog);
    glBindVertexArray(model->vao);

    CAMERA_UNIFORM_MAT(model, proj);
    CAMERA_UNIFORM_MAT(model, view);
    CAMERA_UNIFORM_MAT(model, model);

    THEME_UNIFORM_COLOR(model, key);
    THEME_UNIFORM_COLOR(model, fill);
    THEME_UNIFORM_COLOR(model, base);

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, (void*)0);
}
