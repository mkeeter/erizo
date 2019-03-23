#include "camera.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "shader.h"

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
    model->num_triangles = 0;
    model->vbo = 0;
    glGenVertexArrays(1, &model->vao);

    model->vs = shader_build(MODEL_VS_SRC, GL_VERTEX_SHADER);
    model->gs = shader_build(MODEL_GS_SRC, GL_GEOMETRY_SHADER);
    model->fs = shader_build(MODEL_FS_SRC, GL_FRAGMENT_SHADER);
    model->prog = shader_link_vgf(model->vs, model->gs, model->fs);
    model->u_proj = glGetUniformLocation(model->prog, "proj");
    model->u_view = glGetUniformLocation(model->prog, "view");
    model->u_model = glGetUniformLocation(model->prog, "model");

    model->u_key = glGetUniformLocation(model->prog, "key");
    model->u_fill = glGetUniformLocation(model->prog, "fill");
    model->u_base = glGetUniformLocation(model->prog, "base");

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

void model_draw(model_t* model, camera_t* camera) {
    glEnable(GL_DEPTH_TEST);
    glUseProgram(model->prog);
    glBindVertexArray(model->vao);
    glUniformMatrix4fv(model->u_proj, 1, GL_FALSE, (float*)camera->proj);
    glUniformMatrix4fv(model->u_view, 1, GL_FALSE, (float*)camera->view);
    glUniformMatrix4fv(model->u_model, 1, GL_FALSE, (float*)camera->model);

    const float key[3]  = {0.99f, 0.96f, 0.89f};
    const float fill[3] = {0.92f, 0.91f, 0.83f};
    const float base[3] = {0.40f, 0.48f, 0.51f};
    glUniform3fv(model->u_key,  1, key);
    glUniform3fv(model->u_fill, 1, fill);
    glUniform3fv(model->u_base, 1, base);

    glDrawArrays(GL_TRIANGLES, 0, model->num_triangles * 3);
}
