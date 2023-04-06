#include "app.h"
#include "backdrop.h"
#include "camera.h"
#include "instance.h"
#include "log.h"
#include "platform.h"

instance_t* app_open(app_t* app, const char* filename) {
    if (app->is_reading_stdin) {
      int c = getchar();
      ungetc(c, stdin);
      if (c == 0 || c == -1) {
        return NULL;
      }
    }

    instance_t* instance = instance_new(app, filename, app->draw_proj);

    /*  If loading failed, then do a special one-time drawing of
     *  the backdrop, show an error dialog, and mark the window
     *  as closing in the next event loop */
    if (instance->error) {
        backdrop_draw(instance->backdrop, app->theme);
        glfwSwapBuffers(instance->window);
        platform_warning("Loading the file failed", instance->error);
        glfwSetWindowShouldClose(instance->window, 1);
    }

    if ((app->is_reading_stdin && app->instance_count == 0) || app->instance_count <= 0) {
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
      instance->draw_mode = app->draw_mode;
      app->instances[app->instance_count++] = instance;
    }

    instance_cb_focus(instance, true);
    return instance;
}

void app_defer_open(app_t* app, const char* filename) {
    const size_t len = strlen(filename);
    char* f = calloc(1, len + 1);
    strcpy(f, filename);
    app->deferred_files = realloc(app->deferred_files,
                                  sizeof(char*) * app->deferred_count + 1);
    app->deferred_files[app->deferred_count++] = f;
}

void app_view_shaded(app_t* app) {
    app->draw_mode = DRAW_SHADED;
    for (unsigned i=0; i < app->instance_count; ++i) {
        instance_view_shaded(app->instances[i]);
    }
}

void app_view_wireframe(app_t* app) {
    app->draw_mode = DRAW_WIREFRAME;
    for (unsigned i=0; i < app->instance_count; ++i) {
        instance_view_wireframe(app->instances[i]);
    }
}

void app_view_orthographic(app_t* app) {
    app->draw_proj = CAMERA_PROJ_ORTHOGRAPHIC;
    for (unsigned i=0; i < app->instance_count; ++i) {
        instance_view_orthographic(app->instances[i]);
    }
}

void app_view_perspective(app_t* app) {
    app->draw_proj = CAMERA_PROJ_PERSPECTIVE;
    for (unsigned i=0; i < app->instance_count; ++i) {
        instance_view_perspective(app->instances[i]);
    }
}

void app_set_front(app_t* app, instance_t* instance) {
    for (unsigned i=0; i < app->instance_count; ++i) {
        if (app->instances[i] == instance) {
            memmove(&app->instances[1], &app->instances[0],
                    i * sizeof(instance_t*));
            app->instances[0] = instance;
            return;
        }
    }
    log_error_and_abort("Could not find instance in app_set_front");
}

instance_t* app_get_front(app_t* app) {
    if (app->instances == NULL || app->instance_count == 0) {
        log_error_and_abort("No front instance");
    }
    return app->instances[0];
}

bool app_run(app_t* app) {
    /*  On some platforms, we defer loading of files until the event loop
     *  begins running, so handle them all here. */
    if (app->deferred_files) {
        for (unsigned i=0; i < app->deferred_count; ++i) {
            app_open(app, app->deferred_files[i]);
            if (!app->is_reading_stdin)
                free(app->deferred_files[i]);
        }
        /* Never free stdin because it must be checked constantly. */
        if (!app->is_reading_stdin) {
            free(app->deferred_files);
            app->deferred_files = NULL;
            app->deferred_count = 0;
        }
    }

    bool needs_redraw = false;
    unsigned i = 0;
    while (i < app->instance_count) {
        needs_redraw |= instance_draw(app->instances[i], app->theme);
        if (glfwWindowShouldClose(app->instances[i]->window)) {
            instance_t* target = app->instances[i];
            const bool focused = target->focused;

            /*  Shift the remaining instances by one */
            app->instance_count--;
            for (unsigned j=i; j < app->instance_count; ++j) {
                app->instances[j] = app->instances[j + 1];
            }

            if (focused && app->instance_count) {
                glfwFocusWindow(app->instances[0]->window);
            }

            instance_delete(target);
        } else {
            i++;
        }
    }

    if (needs_redraw) {
        glfwPostEmptyEvent();
    }

    return app->instance_count;
}
