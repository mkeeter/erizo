#include "app.h"
#include "instance.h"
#include "log.h"

int main(int argc, char** argv) {
    log_info("Startup!");

    if (argc != 2) {
        log_info("No input file");
    } else if (argc > 2) {
        log_error_and_abort("Too many arguments (expected 0 or 1)");
    }

    app_t app = { NULL, 0 };
    if (argc == 2) {
        app_open(&app, argv[1]);
    }

    while (1) {
        app_run(&app);
        glfwWaitEvents();
    }

    return 0;
}
