#include "camera.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "shader.h"
#include "theme.h"
#include "wireframe.h"

static const GLchar* WIREFRAME_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

void main() {
    gl_Position = vec4(pos, 1.0f);
}
);

static const GLchar* WIREFRAME_GS_SRC = GLSL(330,
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec3 ec_pos;

void main() {
    vec4 a = gl_in[0].gl_Position;
    vec4 b = gl_in[1].gl_Position;
    vec4 c = gl_in[2].gl_Position;

    gl_Position = proj * view * model * a;
    ec_pos = gl_Position.xyz;
    EmitVertex();

    gl_Position = proj * view * model * b;
    ec_pos = gl_Position.xyz;
    EmitVertex();

    gl_Position = proj * view * model * c;
    ec_pos = gl_Position.xyz;
    EmitVertex();
}
);

static const GLchar* WIREFRAME_FS_SRC = GLSL(330,
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

struct wireframe_ {
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

wireframe_t* wireframe_new() {
    OBJECT_ALLOC(wireframe);
    wireframe->vs = shader_build(WIREFRAME_VS_SRC, GL_VERTEX_SHADER);
    wireframe->gs = shader_build(WIREFRAME_GS_SRC, GL_GEOMETRY_SHADER);
    wireframe->fs = shader_build(WIREFRAME_FS_SRC, GL_FRAGMENT_SHADER);
    wireframe->prog = shader_link_vf(wireframe->vs, wireframe->fs);

    SHADER_GET_UNIFORM_LOC(wireframe, proj);
    SHADER_GET_UNIFORM_LOC(wireframe, view);
    SHADER_GET_UNIFORM_LOC(wireframe, model);

    SHADER_GET_UNIFORM_LOC(wireframe, key);
    SHADER_GET_UNIFORM_LOC(wireframe, fill);
    SHADER_GET_UNIFORM_LOC(wireframe, base);

    log_gl_error();
    return wireframe;
}

void wireframe_delete(wireframe_t* wireframe) {
    glDeleteShader(wireframe->vs);
    glDeleteShader(wireframe->gs);
    glDeleteShader(wireframe->fs);
    glDeleteProgram(wireframe->prog);
    free(wireframe);
}

void wireframe_draw(wireframe_t* wireframe, model_t* model,
                    camera_t* camera, theme_t* theme)
{
    glEnable(GL_DEPTH_TEST);
    glUseProgram(wireframe->prog);
    glBindVertexArray(model->vao);

    CAMERA_UNIFORM_MAT(wireframe, proj);
    CAMERA_UNIFORM_MAT(wireframe, view);
    CAMERA_UNIFORM_MAT(wireframe, model);

    THEME_UNIFORM_COLOR(wireframe, key);
    THEME_UNIFORM_COLOR(wireframe, fill);
    THEME_UNIFORM_COLOR(wireframe, base);

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();
}

