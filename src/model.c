#include "camera.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "shader.h"
#include "theme.h"

static const GLchar* MODEL_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec3 ec_pos;

void main() {
    gl_Position = proj * view * model * vec4(pos, 1.0f);
    ec_pos = gl_Position.xyz;
}
);

static const GLchar* MODEL_FS_SRC = GLSL(330,
in vec3 ec_pos;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

uniform vec3 key;
uniform vec3 fill;
uniform vec3 base;

out vec4 out_color;

void main() {
    vec3 n_screen = cross(dFdx(ec_pos), dFdy(ec_pos));
    n_screen.z *= proj[2][2];
    vec3 normal = normalize(n_screen);
    float a = dot(normal, vec3(0.0f, 0.0f, 1.0f));
    float b = dot(normal, vec3(-0.57f, -0.57f, 0.57f));

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
    model->fs = shader_build(MODEL_FS_SRC, GL_FRAGMENT_SHADER);
    model->prog = shader_link_vf(model->vs, model->fs);

    SHADER_GET_UNIFORM_LOC(model, proj);
    SHADER_GET_UNIFORM_LOC(model, view);
    SHADER_GET_UNIFORM_LOC(model, model);

    SHADER_GET_UNIFORM_LOC(model, key);
    SHADER_GET_UNIFORM_LOC(model, fill);
    SHADER_GET_UNIFORM_LOC(model, base);

    log_gl_error();

    log_trace("Initialized model");
    return model;
}

void model_delete(model_t* model) {
    glDeleteBuffers(1, &model->vbo);
    glDeleteBuffers(1, &model->ibo);
    glDeleteVertexArrays(1, &model->vbo);
    glDeleteShader(model->vs);
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

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();
}
