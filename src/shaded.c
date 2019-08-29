#include "camera.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "shaded.h"
#include "shader.h"
#include "theme.h"

static const GLchar* SHADED_VS_SRC = GLSL(330,
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

static const GLchar* SHADED_FS_SRC = GLSL(330,
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

struct shaded_ {
    /*  Shader program */
    GLuint vs;
    GLuint gs;
    GLuint fs;
    GLuint prog;

    /*  Uniform locations for matrices */
    GLint u_proj;
    GLint u_view;
    GLint u_model;

    /*  Uniform locations for lighting */
    GLint u_key;
    GLint u_fill;
    GLint u_base;
};

shaded_t* shaded_new() {
    OBJECT_ALLOC(shaded);
    shaded->vs = shader_build(SHADED_VS_SRC, GL_VERTEX_SHADER);
    shaded->fs = shader_build(SHADED_FS_SRC, GL_FRAGMENT_SHADER);
    shaded->prog = shader_link_vf(shaded->vs, shaded->fs);

    SHADER_GET_UNIFORM_LOC(shaded, proj);
    SHADER_GET_UNIFORM_LOC(shaded, view);
    SHADER_GET_UNIFORM_LOC(shaded, model);

    SHADER_GET_UNIFORM_LOC(shaded, key);
    SHADER_GET_UNIFORM_LOC(shaded, fill);
    SHADER_GET_UNIFORM_LOC(shaded, base);

    log_gl_error();
    return shaded;
}

void shaded_delete(shaded_t* shaded) {
    glDeleteShader(shaded->vs);
    glDeleteShader(shaded->fs);
    glDeleteProgram(shaded->prog);
    free(shaded);
}

void shaded_draw(shaded_t* shaded, model_t* model,
                 camera_t* camera, theme_t* theme)
{
    glEnable(GL_DEPTH_TEST);
    glUseProgram(shaded->prog);
    glBindVertexArray(model->vao);

    CAMERA_UNIFORM_MAT(shaded, proj);
    CAMERA_UNIFORM_MAT(shaded, view);
    CAMERA_UNIFORM_MAT(shaded, model);

    THEME_UNIFORM_COLOR(shaded, key);
    THEME_UNIFORM_COLOR(shaded, fill);
    THEME_UNIFORM_COLOR(shaded, base);

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();
}
