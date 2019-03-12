#include "app.h"
#include "backdrop.h"
#include "camera.h"
#include "log.h"
#include "loader.h"
#include "model.h"
#include "shader.h"

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

    loader_t loader;
    loader_init(&loader);
    loader_start(&loader, argv[1]);

    /*  Initialize our camera to store width and height */
    camera_t camera;
    camera_update_proj(&camera, 500, 500);

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
            camera.width, camera.height, "hedgehog", NULL, NULL);
    if (!window) {
        log_error_and_abort("Failed to create window");
    }
    log_trace("Created window");

    glfwMakeContextCurrent(window);
    log_trace("Made context current");

    const GLenum glew_err = glewInit();
    if (GLEW_OK != glew_err) {
        log_error_and_abort("GLEW initialization failed: %s\n",
                            glewGetErrorString(glew_err));
    }
    log_trace("Initialized GLEW");

    /*  Highest priority once OpenGL is running: allocate the VBO
     *  and pass it to the loader thread.  */
    loader_allocate_vbo(&loader);

    /*  Next, build the OpenGL-dependent objects */
    model_t model;
    model_init(&model);

    backdrop_t backdrop;
    backdrop_init(&backdrop);

    int first = 1;
    glfwShowWindow(window);

    app_t app = {
        &backdrop,
        &camera,
        &loader,
        &model,
    };

    glfwSetWindowUserPointer(window, &app);
    glfwSetKeyCallback(window, key_callback);
    log_trace("Showed window");

    glClearDepth(1.0);
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_DEPTH_BUFFER_BIT);

        /*  On first opening the window, hold until the model is
         *  ready (rather than continuing to render the empty backdrop */
        if (first) {
            loader_wait(&loader, LOADER_DONE);
        }
        if (loader_state(&loader) == LOADER_DONE) {
            loader_finish(&loader, &model);
        }

        backdrop_draw(&backdrop);
        model_draw(&model, camera.proj);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        if (first) {
            log_info("First draw complete");
        }

        /* Poll for and process events */
        glfwWaitEvents();
        first = 0;
    }

    return 0;
}
