#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "log.h"
#include "loader.h"
#include "worker.h"
#include "model.h"

/******************************************************************************/

#define GLSL(version, shader)  "#version " #version "\n" #shader

const GLchar* MODEL_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

void main() {
    gl_Position = vec4(pos, 1.0);
}
);

const GLchar* MODEL_GS_SRC = GLSL(330,
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 proj;
uniform mat4 model;

out vec3 vert_norm;
out vec3 pos_bary;

void main() {
    vec3 a = gl_in[0].gl_Position.xyz;
    vec3 b = gl_in[1].gl_Position.xyz;
    vec3 c = gl_in[2].gl_Position.xyz;

    vec3 na = cross(a - b, c - b);
    vec3 nb = cross(b - c, a - c);
    vec3 nc = cross(c - a, b - a);

    vert_norm = normalize(na + nb + nc);

    pos_bary = vec3(1.0, 0.0, 0.0);
    gl_Position = proj * model * gl_in[0].gl_Position;
    EmitVertex();

    pos_bary = vec3(0.0, 1.0, 0.0);
    gl_Position = proj * model * gl_in[1].gl_Position;
    EmitVertex();

    pos_bary = vec3(0.0, 0.0, 1.0);
    gl_Position = proj * model * gl_in[2].gl_Position;
    EmitVertex();
}
);

const GLchar* MODEL_FS_SRC = GLSL(330,
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
    log_info("Startup!");

    if (argc != 2) {
        log_error_and_abort("No input file");
    }

    model_t model;
    loader_t loader;
    loader.filename = argv[1];
    loader.buffer = NULL;
    loader.model = &model;
    loader.state = LOADER_START;
    platform_mutex_init(&loader.mutex);
    platform_cond_init(&loader.cond);

    platform_thread_t loader_thread;
    if (platform_thread_create(&loader_thread, loader_run, &loader)) {
        log_error_and_abort("Error creating thread");
    }

    if (!glfwInit()) {
        log_error_and_abort("Failed to initialize glfw");
    }

    glfwWindowHint(GLFW_SAMPLES, 8);    /* multisampling! */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* const window = glfwCreateWindow(
            500, 500, "hedgehog", NULL, NULL);

    if (!window) {
        log_error_and_abort("Failed to create window");
    }
    log_trace("Created window");

    glfwMakeContextCurrent(window);
    log_trace("Made context current");

    {
        const GLenum err = glewInit();
        if (GLEW_OK != err) {
            log_error_and_abort("GLEW initialization failed: %s\n",
                                glewGetErrorString(err));
        }
    }
    log_trace("Initialized GLEW");

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    /*  Allocate data for the model, then send it to the loader thread */
    loader_wait(&loader, LOADER_GOT_SIZE);
    glBufferData(GL_ARRAY_BUFFER, model.num_triangles * 36,
                 NULL, GL_STATIC_DRAW);
    loader.buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    loader_next(&loader, LOADER_GOT_BUFFER);
    log_trace("Allocated buffer");

    glfwSetKeyCallback(window, key_callback);

    GLuint vs = compile_shader(MODEL_VS_SRC, GL_VERTEX_SHADER);
    GLuint gs = compile_shader(MODEL_GS_SRC, GL_GEOMETRY_SHADER);
    GLuint fs = compile_shader(MODEL_FS_SRC, GL_FRAGMENT_SHADER);
    GLuint prog = link_program(vs, gs, fs);

    glUseProgram(prog);
    GLuint loc_proj = glGetUniformLocation(prog, "proj");
    GLuint loc_model = glGetUniformLocation(prog, "model");
    log_trace("Compiled shaders");

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    log_trace("Assigned attribute pointers");

    int first = 1;
    glfwShowWindow(window);
    log_trace("Showed window");
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.3, 0.3, 0.3, 1.0);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        if (first) {
            if (platform_thread_join(&loader_thread)) {
                log_error_and_abort("Failed to join loader thread");
            }
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }

        const float proj[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f};
        glUniformMatrix4fv(loc_proj, 1, GL_FALSE, proj);

        glUniformMatrix4fv(loc_model, 1, GL_FALSE, model.mat);

        glDrawArrays(GL_TRIANGLES, 0, model.num_triangles * 3);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        if (first) {
            log_info("First draw complete");
        }

        /* Poll for and process events */
        glfwPollEvents();

        first = 0;
    }

    return 0;
}
