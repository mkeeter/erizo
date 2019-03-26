#include "app.h"
#include "backdrop.h"
#include "instance.h"
#include "log.h"

static void app_close_instance(app_t* app, unsigned i) {
    if (i >= app->instance_count) {
        log_error_and_abort("Invalid index (%u > %u)",
                            i, app->instance_count);
    }
    instance_delete(app->instances[i]);

    /*  Shift the remaining instances by one */
    app->instance_count--;
    while (i < app->instance_count) {
        app->instances[i] = app->instances[i + 1];
        i++;
    }
}

instance_t* app_open(app_t* app, const char* filename) {
    instance_t* instance = instance_new(app, filename);

    /*  If loading failed, then do a special one-time drawing of
     *  the backdrop, show an error dialog, and mark the window
     *  as closing in the next event loop */
    if (instance->error) {
        backdrop_draw(instance->backdrop, app->theme);
        glfwSwapBuffers(instance->window);
        platform_warning("Loading the file failed", instance->error);
        glfwSetWindowShouldClose(instance->window, 1);
    }

    /*  Add this instance at the back of the array */
    if (app->instance_count == app->instances_size) {
        if (app->instances_size) {
            app->instances_size *= 2;
        } else {
            app->instances_size = 1;
        }
        app->instances = (instance_t**)realloc(
                app->instances, sizeof(instance_t*) * app->instances_size);
    }
    app->instances[app->instance_count++] = instance;
    return instance;
}

bool app_run(app_t* app) {
    bool any_active = false;
    for (unsigned i=0; i < app->instance_count; ++i) {
        instance_draw(app->instances[i], app->theme);
        if (glfwWindowShouldClose(app->instances[i]->window)) {
            app_close_instance(app, i);
        } else {
            any_active = true;
        }
    }
    return any_active;
}
