#include "app.h"
#include "instance.h"
#include "log.h"
#include "window.h"

int main(int argc, char** argv) {
    log_info("Startup!");
    app_t app = { NULL, 0 };
    platform_preinit(&app);

    if (argc != 2) {
        log_info("No input file");
    } else if (argc > 2) {
        log_error_and_abort("Too many arguments (expected 0 or 1)");
    }

    platform_init(&app, argc, argv);

    while (app_run(&app)) {
        glfwWaitEvents();
    }

    return 0;
}
