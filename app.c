#include "app.h"
#include "backdrop.h"
#include "camera.h"
#include "loader.h"
#include "log.h"
#include "mat.h"
#include "model.h"

void app_cb_keypress(app_t* app, int key, int scancode,
                     int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        app->state = APP_QUIT;
    }
}

void app_cb_window_size(app_t* app, int width, int height)
{
    app->camera->width = width;
    app->camera->height = height;
    camera_update_proj(app->camera);

    /*  Continue to render while the window is being resized
     *  (otherwise it ends up greyed out on Mac)  */
    app_run(app);
}

void app_cb_mouse_pos(app_t* app, float xpos, float ypos) {
    camera_set_mouse_pos(app->camera, xpos, ypos);
}

void app_cb_mouse_click(app_t* app, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_1) {
            camera_begin_pan(app->camera);
        } else if (button == GLFW_MOUSE_BUTTON_2) {
            camera_begin_rot(app->camera);
        }
    } else {
        camera_end_drag(app->camera);
    }
}
void app_cb_mouse_scroll(app_t* app, float xoffset, float yoffset) {
}

void app_init(app_t* app) {
    /*  We populated camera width + height, but didn't yet build
     *  the projection or view matrices (to save time).  */
    camera_update_proj(app->camera);
    camera_update_view(app->camera);

    glClearDepth(1.0);
}

void app_run(app_t* app) {
    glClear(GL_DEPTH_BUFFER_BIT);
    backdrop_draw(app->backdrop);

    /*  At the very last moment, check on the loader */
    if (app->state == APP_LOAD) {
        loader_wait(app->loader, LOADER_DONE);
        app->state = APP_LOAD;
        loader_finish(app->loader, app->model, app->camera);
    }
    model_draw(app->model, app->camera);
    glfwSwapBuffers(app->window);

    /*  Print a timing message on first load */
    if (app->state == APP_LOAD) {
        log_info("Load complete");
        loader_reset(app->loader);
        app->state = APP_RUNNING;
    }
}

void app_open(app_t* app, const char* filename) {
    if (app->state != APP_RUNNING) {
        log_error_and_abort("Invalid state for app_open: %i", app->state);
    }
    loader_start(app->loader, filename);
    loader_allocate_vbo(app->loader);
    app->state = APP_LOAD;
}
