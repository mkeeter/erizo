#include "draw.h"
#include "shaded.h"
#include "shader.h"

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

draw_t* shaded_new() {
    return draw_new(SHADED_VS_SRC, NULL, SHADED_FS_SRC);
}
