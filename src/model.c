#include "ao.h"
#include "camera.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "shader.h"
#include "theme.h"

static const GLchar* MODEL_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

void main() {
    gl_Position = vec4(pos, 1.0f);
}
);

static const GLchar* MODEL_GS_SRC = GLSL(330,
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec3 vert_norm;
out vec3 pos_bary;
out vec3 pos_model;

void main() {
    vec3 a = gl_in[0].gl_Position.xyz;
    vec3 b = gl_in[1].gl_Position.xyz;
    vec3 c = gl_in[2].gl_Position.xyz;

    vec3 na = cross(a - b, c - b);
    vec3 nb = cross(b - c, a - c);
    vec3 nc = cross(c - a, b - a);
    vert_norm = normalize((view * model * vec4(na + nb + nc, 0.0f)).xyz);

    mat4 pv = proj * view;
    vec4 pm;

    pos_bary = vec3(1.0f, 0.0f, 0.0f);
    pm = model * gl_in[0].gl_Position;
    pos_model = pm.xyz;
    gl_Position = pv * pm;
    EmitVertex();

    pos_bary = vec3(0.0f, 1.0f, 0.0f);
    pm = model * gl_in[1].gl_Position;
    pos_model = pm.xyz;
    gl_Position = pv * pm;
    EmitVertex();

    pos_bary = vec3(0.0f, 0.0f, 1.0f);
    pm = model * gl_in[2].gl_Position;
    pos_model = pm.xyz;
    gl_Position = pv * pm;
    EmitVertex();
}
);

static const GLchar* MODEL_FS_SRC = GLSL(330,
in vec3 vert_norm;
in vec3 pos_bary;
in vec3 pos_model;

uniform vec3 key;
uniform vec3 fill;
uniform vec3 base;

uniform sampler2D vol_tex;
uniform int vol_logsize;
uniform int vol_num_rays;

out vec4 out_color;

void main() {
    float a = dot(vert_norm, vec3(0.0f, 0.0f, 1.0f));
    float b = dot(vert_norm, vec3(-0.57f, -0.57f, 0.57f));

    if (vol_num_rays != 0) {
        int size = (1 << vol_logsize);
        int tiles = (1 << (vol_logsize / 2));

        // Normalize position to voxel scale
        vec3 pos_norm = (pos_model + 1.0f) / 2.0f;
        int z = int(pos_norm.z * size);
        float zx = float(z / tiles);
        float zy = float(z % tiles);

        float tx = pos_norm.x / size + zx / tiles;
        float ty = pos_norm.y / size + zy / tiles;
        vec4 t = texture(vol_tex, vec2(tx, ty));
        vec3 c = normalize(t.xyz) / 2.0f + 0.5f;

        out_color = vec4(c * t.w / vol_num_rays, 1.0f);
    } else {
        out_color = vec4(mix(base, key,  a) * 0.5f +
                         mix(base, fill, b) * 0.5f, 1.0f);
    }
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

    SHADER_GET_UNIFORM_LOC(model, vol_tex);
    SHADER_GET_UNIFORM_LOC(model, vol_logsize);
    SHADER_GET_UNIFORM_LOC(model, vol_num_rays);
    log_gl_error();

    log_trace("Initialized model");
    return model;
}

void model_delete(model_t* model) {
    glDeleteBuffers(1, &model->vbo);
    glDeleteVertexArrays(1, &model->vao);
    glDeleteVertexArrays(1, &model->vbo);
    glDeleteShader(model->vs);
    glDeleteShader(model->gs);
    glDeleteShader(model->fs);
    glDeleteProgram(model->prog);
    free(model);
}

void model_draw(model_t* model, ao_t* ao, camera_t* camera, theme_t* theme) {
    glEnable(GL_DEPTH_TEST);
    glUseProgram(model->prog);
    glBindVertexArray(model->vao);

    CAMERA_UNIFORM_MAT(model, proj);
    CAMERA_UNIFORM_MAT(model, view);
    CAMERA_UNIFORM_MAT(model, model);

    THEME_UNIFORM_COLOR(model, key);
    THEME_UNIFORM_COLOR(model, fill);
    THEME_UNIFORM_COLOR(model, base);

    // Activate environmental lighting / ambient occlusion
    if (ao) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ao->vol.tex[ao->vol.pingpong]);
        glUniform1i(model->u_vol_tex, 0);
        glUniform1i(model->u_vol_logsize, ao->vol.logsize);
        glUniform1i(model->u_vol_num_rays, ao->vol.rays);
    } else {
        glUniform1i(model->u_vol_logsize, 0);
        glUniform1i(model->u_vol_num_rays, 0);
    }

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();
}
