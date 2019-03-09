#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "platform.h"
#include "platform_unix.h"

/******************************************************************************/

#define GLSL(version, shader)  "#version " #version "\n" #shader

const GLchar* RAW_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

void main() {
    gl_Position = vec4(pos, 1.0);
}
);

const GLchar* RAW_GS_SRC = GLSL(330,
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

void trace(const char *fmt, ...)
{
    static int64_t start_sec = -1;
    static int32_t start_usec = -1;

    if (start_sec == -1) {
        platform_get_time(&start_sec, &start_usec);
    }

    int64_t now_sec;
    int32_t now_usec;
    platform_get_time(&now_sec, &now_usec);

    long int dt_sec = now_sec - start_sec;
    int dt_usec = now_usec - start_usec;
    if (dt_usec < 0) {
        dt_usec += 1000000;
        dt_sec -= 1;
    }

    va_list args;
    va_start(args, fmt);
    printf("[hedgehog] (%li.%06i) ", dt_sec, dt_usec);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

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

typedef struct {
    GLuint num_triangles;
    float mat[16];
} model_t;

typedef enum {
    LOADER_START,
    LOADER_GOT_SIZE,
    LOADER_GOT_BUFFER,
} loader_state_t;

typedef struct {
    const char* filename;
    const char* mapped;
    float* buffer; /* Mapped by OpenGL */

    /*  Synchronization system with the main thread */
    loader_state_t state;
    platform_mutex_t mutex;
    platform_cond_t cond;

    model_t* model;
} loader_t;

typedef struct {
    loader_t* loader;
    uint32_t start;
    uint32_t end;
} worker_t;

void loader_wait(loader_t* loader, loader_state_t target) {
    while (loader->state != target) {
        platform_mutex_lock(&loader->mutex);
        platform_cond_wait(&loader->cond, &loader->mutex);
        platform_mutex_unlock(&loader->mutex);
    }
}

void loader_next(loader_t* loader, loader_state_t target) {
    platform_mutex_lock(&loader->mutex);
        loader->state = target;
        platform_cond_broadcast(&loader->cond);
    platform_mutex_unlock(&loader->mutex);
}

void* worker_run(void* worker_) {
    worker_t* const worker = (worker_t*)worker_;
    loader_t* const loader = worker->loader;

    loader_wait(loader, LOADER_GOT_BUFFER);

    /*  Copy data to the GPU buffer */
    size_t index = 80 + 4 + 3 * sizeof(float);
    const size_t index_stride = 12 * sizeof(float) + sizeof(uint16_t);
    index += index_stride * worker->start;
    for (uint32_t t=worker->start; t < worker->end; t += 1) {
        memcpy(&loader->buffer[t * 9], &loader->mapped[index], 9 * sizeof(float));
        index += index_stride;
    }

    return NULL;
}

void* load_model(void* loader_) {
    loader_t* const loader = (loader_t*)loader_;
    model_t* const m = loader->model;

    loader->mapped = platform_mmap(loader->filename);
    memcpy(&m->num_triangles, &loader->mapped[80], sizeof(m->num_triangles));
    loader_next(loader, LOADER_GOT_SIZE);

    /*  We kick off our mmap-to-OpenGL copying workers here, even though the
     *  buffer won't be ready for a little while.  This means that as soon
     *  as the buffer is ready, they'll start! */
    const size_t NUM_WORKERS = 8;
    worker_t workers[NUM_WORKERS];
    platform_thread_t worker_threads[NUM_WORKERS];
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        workers[i].loader = loader;
        workers[i].start = i * m->num_triangles / NUM_WORKERS;
        workers[i].end = (i + 1) * m->num_triangles / NUM_WORKERS;
        if (platform_thread_create(&worker_threads[i], worker_run,
                                   &workers[i]))
        {
            fprintf(stderr, "[hedgehog]    Error creating worker thread\n");
            exit(1);
        }
    }

    size_t index = 80 + 4 + 3 * sizeof(float);
    float min[3];
    float max[3];
    memcpy(min, &loader->mapped[index], sizeof(min));
    memcpy(min, &loader->mapped[index], sizeof(max));

    for (uint32_t t=0; t < m->num_triangles; t += 1) {
        float xyz[9];
        memcpy(xyz, &loader->mapped[index], 9 * sizeof(float));
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

    float center[3];
    float scale = 0;
    for (i=0; i < 3; ++i) {
        center[i] = (min[i] + max[i]) / 2.0f;
        const float d = max[i] - min[i];
        if (d > scale) {
            scale = d;
        }
    }

    memset(m->mat, 0, sizeof(m->mat));
    m->mat[0] = 1.0f / scale;
    m->mat[5] = 1.0f / scale;
    m->mat[10] = 1.0f / scale;
    m->mat[15] = 1.0f;

    trace("Waiting for buffer...");
    loader_wait(loader, LOADER_GOT_BUFFER);
    trace("Got buffer in loader thread");

    for (i = 0; i < NUM_WORKERS; ++i) {
        if (platform_thread_join(&worker_threads[i])) {
            fprintf(stderr, "[hedgehog]    Error joining worker thread\n");
        }
    }

    trace("Loader thread done");
    return NULL;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char** argv) {
    trace("Startup!");

    if (argc != 2) {
        fprintf(stderr, "[hedgehog]    No input file\n");
        return -1;
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
    if (platform_thread_create(&loader_thread, load_model, &loader)) {
        fprintf(stderr, "[hedgehog]    Error creating thread\n");
        return 1;
    }

    if (!glfwInit())    return -1;

    glfwWindowHint(GLFW_SAMPLES, 8);    /* multisampling! */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* const window = glfwCreateWindow(
            500, 500, "hedgehog", NULL, NULL);

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

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    /*  Allocate data for the model, then send it to the loader thread */
    loader_wait(&loader, LOADER_GOT_SIZE);
    glBufferData(GL_ARRAY_BUFFER, model.num_triangles * 36,
                 NULL, GL_STATIC_DRAW);
    loader.buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    loader_next(&loader, LOADER_GOT_BUFFER);
    trace("Allocated buffer");

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

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    trace("Assigned attribute pointers");

    int first = 1;
    glfwShowWindow(window);
    trace("Showed window");
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.3, 0.3, 0.3, 1.0);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        if (first) {
            if (platform_thread_join(&loader_thread)) {
                fprintf(stderr, "[hedgehog]    Error joining thread\n");
                return 2;
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
            trace("First draw complete");
        }

        /* Poll for and process events */
        glfwPollEvents();

        first = 0;
    }

    return 0;
}
