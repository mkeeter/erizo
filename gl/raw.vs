#version 330
layout(location=0) in vec3 a;
layout(location=1) in vec3 b;
layout(location=2) in vec3 c;

out vec4 vb;
out vec4 vc;

void main() {
    gl_Position = vec4(b, 1.0);
    vb = vec4(c, 1.0);
    vc = vec4(a, 1.0);
}
