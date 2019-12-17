#include "draw.h"
#include "wireframe.h"
#include "shader.h"

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
out vec3 edge_dist;

float distance(vec3 pt, vec3 a, vec3 b) {
    vec3 dt = normalize(b - a);
    float dist = dot(pt - a, dt);
    vec3 proj = a + dist * dt;
    return length(proj - pt);
}

void main() {
    vec4 a = view * model * gl_in[0].gl_Position;
    vec4 b = view * model * gl_in[1].gl_Position;
    vec4 c = view * model * gl_in[2].gl_Position;

    gl_Position = proj * a;
    ec_pos = gl_Position.xyz;
    edge_dist = vec3(0.0f, distance(a.xyz, b.xyz, c.xyz), 0.0f);
    EmitVertex();

    gl_Position = proj * b;
    ec_pos = gl_Position.xyz;
    edge_dist = vec3(0.0f, 0.0f, distance(b.xyz, c.xyz, a.xyz));
    EmitVertex();

    gl_Position = proj * c;
    ec_pos = gl_Position.xyz;
    edge_dist = vec3(distance(c.xyz, a.xyz, b.xyz), 0.0f, 0.0f);
    EmitVertex();
}
);

static const GLchar* WIREFRAME_FS_SRC = GLSL(330,
in vec3 ec_pos;
in vec3 edge_dist;

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

    float d = min(edge_dist.x, min(edge_dist.y, edge_dist.z));
    float s = clamp(d / 0.002f, 0.8f, 1.0f);
    out_color = vec4(mix(base, key,  a) * 0.5f * s +
                     mix(base, fill, b) * 0.5f * s, 1.0f);
}
);

draw_t* wireframe_new() {
    return draw_new(WIREFRAME_VS_SRC, WIREFRAME_GS_SRC, WIREFRAME_FS_SRC);
}
