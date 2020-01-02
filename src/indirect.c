#include "indirect.h"
#include "camera.h"
#include "shader.h"
#include "object.h"

static const GLchar* INDIRECT_VS_SRC = GLSL(330,
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

static const GLchar* INDIRECT_FS_SRC = GLSL(330,
in vec3 ec_pos;

uniform mat4 proj;

out float out_depth;
out vec4 out_color;

void main() {
    vec3 n_screen = cross(dFdx(ec_pos), dFdy(ec_pos));
    n_screen.z *= proj[2][2];
    vec3 normal = normalize(n_screen);

    out_depth = gl_FragDepth;
    out_color = vec4(normal, 1.0f);
}
);

struct indirect_ {
    camera_uniforms_t u_camera;
    shader_t shader;
};

indirect_t* indirect_new(uint32_t width, uint32_t height) {
    OBJECT_ALLOC(indirect);
    indirect->shader = shader_new(INDIRECT_VS_SRC, NULL, INDIRECT_FS_SRC);

    return indirect;
}
