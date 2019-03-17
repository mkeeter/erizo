#include "app.h"
#include "instance.h"
#include "log.h"
#include "window.h"

int main(int argc, char** argv) {
    log_info("Startup!");

    if (argc != 2) {
        log_info("No input file");
    } else if (argc > 2) {
        log_error_and_abort("Too many arguments (expected 0 or 1)");
    }

    app_t app = { NULL, 0 };
    GLFWwindow* window = NULL;
    if (argc == 2) {
        app_open(&app, argv[1]);
    } else {
        /*  Create a new, invisible window so that the menu bar
         *  has something to bind to in platform_init */
        window = window_new(100.0f, 100.0f);
        glfwShowWindow(window);
    }
    platform_init(&app);
    if (window) {
        glfwPollEvents();
        glfwDestroyWindow(window);
        log_trace("Hiding window");
    }

    while (1) {
        app_run(&app);
        glfwWaitEvents();
    }

    return 0;
}
