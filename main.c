#include "app.h"
#include "backdrop.h"
#include "camera.h"
#include "log.h"
#include "loader.h"
#include "mat.h"
#include "model.h"
#include "shader.h"
#include "window.h"

int main(int argc, char** argv) {
    log_info("Startup!");

    if (argc != 2) {
        log_info("No input file");
    } else if (argc > 2) {
        log_error_and_abort("Too many arguments (expected 0 or 1)");
    }
    const app_state_t state = (argc == 2) ? APP_LOAD : APP_RUNNING;

    loader_t loader;
    loader_init(&loader);

    if (state == APP_LOAD) {
        loader_start(&loader, argv[1]);
    }

    /*  Initialize our camera to store width and height */
    camera_t camera = {500, 500, .yaw=0.8f};

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
    if (state == APP_LOAD) {
        loader_allocate_vbo(&loader);
    }

    /*  Next, build the OpenGL-dependent objects */
    model_t model;
    model_init(&model);

    backdrop_t backdrop;
    backdrop_init(&backdrop);

    glfwShowWindow(window);
    log_trace("Showed window");

    /*  Build the app by stitching together our other objects */
    app_t app = { state, &backdrop, &camera, &loader, &model, window };
    app_init(&app);
    window_set_callbacks(window, &app);
    platform_build_menus(&app);

    while (app.state != APP_QUIT && !glfwWindowShouldClose(window)) {
        app_run(&app);
        glfwWaitEvents();
    }

    return 0;
}
