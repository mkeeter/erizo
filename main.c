#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>

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

void trace(const char *fmt, ...)
{
    static struct timeval start = {-1, -1};

    if (start.tv_sec == -1) {
        gettimeofday(&start, NULL);
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    long int dt_sec = now.tv_sec - start.tv_sec;\
    int dt_usec = now.tv_usec - start.tv_usec;\
    if (dt_usec < 0) {
        dt_usec += 1000000;
        dt_sec -= 1;
    }
    printf("[hedgehog] (%li.%06i) ", dt_sec, dt_usec);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

int main(int argc, char** argv) {
    trace("Startup!");

    if (argc != 2) {
        fprintf(stderr, "[hedgehog]    No input file\n");
        return -1;
    }

    int stl_fd = open(argv[1], O_RDONLY);
    struct stat s;
    fstat(stl_fd, &s);
    size_t size = s.st_size;

    const char* mapped = mmap(0, size, PROT_READ, MAP_PRIVATE, stl_fd, 0);

    uint32_t num_triangles;
    memcpy(&num_triangles, mapped + 80, sizeof(num_triangles));
    trace("Got %i triangles", num_triangles);

    float min[3];
    float max[3];
    size_t index = 80 + 4 + 3 * sizeof(float);
    memcpy(min, &mapped[index], sizeof(min));
    memcpy(max, &mapped[index], sizeof(max));
    for (uint32_t t=0; t < num_triangles; t += 1) {
        float xyz[9];
        memcpy(xyz, &mapped[index], 9 * sizeof(float));
        for (unsigned i=0; i < 3; ++i) {
            for (unsigned a=0; a < 3; ++a) {
                const float v = xyz[3 * i + a];
                if (v < min[a]) {
                    min[a] = v;
                }
                if (v > max[a]) {
                    max[a] = v;
                }
            }
        }
        index += 12 * sizeof(float) + sizeof(uint16_t);
    }
    trace("Got bounds [%f %f %f] [%f %f %f]",
          min[0], min[1], min[2],
          max[0], max[1], max[2]);

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
    trace("Created window");

    glfwMakeContextCurrent(window);
    trace("Made context current");

    {
        const GLenum err = glewInit();
        if (GLEW_OK != err) {
            fprintf(stderr, "GLEW initialization failed: %s\n",
                    glewGetErrorString(err));
            return -1;
        }
    }
    trace("Initialized GLEW");

    glfwSetKeyCallback(window, key_callback);

    GLuint vs = compile_shader(RAW_VS_SRC, GL_VERTEX_SHADER);
    GLuint gs = compile_shader(RAW_GS_SRC, GL_GEOMETRY_SHADER);
    GLuint fs = compile_shader(RAW_FS_SRC, GL_FRAGMENT_SHADER);
    GLuint prog = link_program(vs, gs, fs);

    glUseProgram(prog);
    GLuint loc_proj = glGetUniformLocation(prog, "proj");
    GLuint loc_model = glGetUniformLocation(prog, "model");
    trace("Compiled shaders");

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, num_triangles * 50,
                 &mapped[84], GL_STATIC_DRAW);
    trace("Loaded buffer data");

    for (unsigned i=0; i < 3; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 50,
                              (const void*)(12UL * (i + 1)));
    }
    trace("Assigned attribute pointers");

    int first = 1;
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.3, 0.3, 0.3, 1.0);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDrawArrays(GL_POINTS, 0, num_triangles);

        float M[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f};
        glUniformMatrix4fv(loc_proj, 1, GL_FALSE, M);
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, M);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        if (first) { trace("First draw complete"); }

        /* Poll for and process events */
        glfwPollEvents();

        first = 0;
    }

    return 0;
}
