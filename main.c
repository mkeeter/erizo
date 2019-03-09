#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

/******************************************************************************/

#define GLSL(version, shader)  "#version " #version "\n" #shader

const GLchar* RAW_VS_SRC = GLSL(330,
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
);

const GLchar* RAW_GS_SRC = GLSL(330,
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
);

const GLchar* RAW_FS_SRC = GLSL(330,
in vec3 vert_norm;
in vec3 pos_bary;

out vec4 out_color;

void main() {
    out_color = vec4(abs(vert_norm), 1.0) * 0.8
              + vec4(pos_bary, 1.0) * 0.2;
}
);

/******************************************************************************/

GLuint compile_shader(const GLchar* src, GLenum type) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        GLchar* buf = malloc(len + 1);
        glGetShaderInfoLog(shader, len, NULL, buf);
        fprintf(stderr, "Failed to build shader: %s\n", buf);
        free(buf);
    }

    return shader;
}

GLuint link_program(GLuint vs, GLuint gs, GLuint fs) {
    GLuint program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, gs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

        GLchar* buf = malloc(len + 1);
        glGetProgramInfoLog(program, len, NULL, buf);
        fprintf(stderr, "Failed to link program: %s\n", buf);
        free(buf);
    }

    return program;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char** argv) {
    struct timeval stop, start;
    gettimeofday(&start, NULL);

    if (argc != 2) {
        fprintf(stderr, "[hedgehog]    No input file\n");
        return -1;
    }

    FILE* stl = fopen(argv[1], "r");
    fseek(stl, 80, SEEK_SET);
    uint32_t num_triangles;
    fread(&num_triangles, sizeof(num_triangles), 1, stl);
    fseek(stl, 3 * sizeof(float), SEEK_CUR);

    float* triangles = malloc(sizeof(float) * 3 * 3 * num_triangles);
    float min[3];
    float max[3];
    for (uint32_t t=0; t < num_triangles; ++t) {
        fread(&triangles[t * 9], sizeof(float), 9, stl);
        fseek(stl, sizeof(uint16_t) + 3 * sizeof(float), SEEK_CUR);
        if (t == 0) {
            for (unsigned a=0; a < 3; ++a) {
                min[a] = triangles[a];
                max[a] = triangles[a];
            }
        }
        for (unsigned i=0; i < 3; ++i) {
            for (unsigned a=0; a < 3; ++a) {
                const float v = triangles[3 * i + a + 9 * t];
                if (v < min[a]) {
                    min[a] = v;
                }
                if (v > max[a]) {
                    max[a] = v;
                }
            }
        }
    }

    if (!glfwInit())    return -1;

    glfwWindowHint(GLFW_SAMPLES, 8);    /* multisampling! */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* const window = glfwCreateWindow(
            640, 480, "hedgehog", NULL, NULL);

    if (!window) {
        fprintf(stderr, "[hedgehog]    Error: failed to create window!\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    {
        const GLenum err = glewInit();
        if (GLEW_OK != err) {
            fprintf(stderr, "GLEW initialization failed: %s\n",
                    glewGetErrorString(err));
            return -1;
        }
    }

    glfwSetKeyCallback(window, key_callback);

    GLuint vs = compile_shader(RAW_VS_SRC, GL_VERTEX_SHADER);
    GLuint gs = compile_shader(RAW_GS_SRC, GL_GEOMETRY_SHADER);
    GLuint fs = compile_shader(RAW_FS_SRC, GL_FRAGMENT_SHADER);
    GLuint prog = link_program(vs, gs, fs);

    glUseProgram(prog);

    int first = 1;
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.3, 0.3, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        if (first) {
            first = 0;
            gettimeofday(&stop, NULL);
            printf("took %i\n", stop.tv_usec - start.tv_usec);
        }
    }

    return 0;
}
