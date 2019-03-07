#version 330
layout (points) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 proj;
uniform mat4 model;

in vec4 vb[1];
in vec4 vc[1];

out vec3 vert_norm;
out vec3 pos_bary;

void main() {
    vec3 a = gl_in[0].gl_Position.xyz;
    vec3 b = vb[0].xyz;
    vec3 c = vc[0].xyz;

    vec3 na = cross(a - b, c - b);
    vec3 nb = cross(b - c, a - c);
    vec3 nc = cross(c - a, b - a);

    vert_norm = normalize(na + nb + nc);

    pos_bary = vec3(1.0, 0.0, 0.0);
    gl_Position = proj * model * gl_in[0].gl_Position;
    EmitVertex();

    pos_bary = vec3(0.0, 1.0, 0.0);
    gl_Position = proj * model * vb[0];
    EmitVertex();

    pos_bary = vec3(0.0, 0.0, 1.0);
    gl_Position = proj * model * vc[0];
    EmitVertex();
}

