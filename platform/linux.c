#include "app.h"
#include "log.h"
#include "window.h"

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

static app_t* app_handle = NULL;
void platform_init(app_t* app, int argc, char** argv) {
    app_handle = app;
    if (argc == 2) {
        // Defer loading stl from stdin. It has to happen in the main event loop
        // to avoid creating new windows.
        app->is_reading_stdin = !strcmp(argv[1], "-");
        if (app->is_reading_stdin) {
            app_defer_open(app, argv[1]);
        } else {
            app_open(app, argv[1]);
        }
    }
    if (app->instance_count == 0) {
        //  Construct a dummy window, which triggers GLFW initialization
        //  and may cause the application to open a file (if it was
        //  double-clicked or dragged onto the icon).
        window_new("", 1.0f, 1.0f);

        if (app->instance_count == 0) {
            app_open(app, ":/sphere");
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void platform_window_bind(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(
        window, framebuffer_size_callback);
}

/*  Shows a warning dialog with the given text */
void platform_warning(const char* title, const char* text) {
    log_error("%s", text);
}
