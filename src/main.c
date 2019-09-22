#include "app.h"
#include "instance.h"
#include "theme.h"
#include "log.h"
#include "platform.h"
#include "window.h"

int main(int argc, char** argv) {
    log_info("Startup!");
    app_t app = {
        .instances=NULL,
        .instance_count=0,
        .instances_size=0,
        .theme=NULL,
        .draw_mode=DRAW_SHADED,
    };
    app.theme = theme_new_solarized();

    if (argc != 2) {
        log_info("No input file");
    } else if (argc > 2) {
        log_error_and_abort("Too many arguments (expected 0 or 1)");
    }

    /* Platform-specific initialization (which includes loading files
     * from the command line, since there are subtleties involved) */
    platform_init(&app, argc, argv);

    while (app_run(&app)) {
        glfwWaitEvents();
    }

    return 0;
}
