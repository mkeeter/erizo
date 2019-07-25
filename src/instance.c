#include "app.h"
#include "backdrop.h"
#include "camera.h"
#include "instance.h"
#include "loader.h"
#include "log.h"
#include "mat.h"
#include "model.h"
#include "object.h"
#include "platform.h"
#include "theme.h"
#include "window.h"

instance_t* instance_new(app_t* parent, const char* filepath) {
    /*  Kick the loader off in a separate thread */
    loader_t* loader = loader_new(filepath);

    camera_t* camera = camera_new(500.0f, 500.0f);
    const char* filename = platform_filename(filepath);
    GLFWwindow* window = window_new(filename, camera->width, camera->height);

    /*  Highest priority once OpenGL is running: allocate the VBO
     *  and pass it to the loader thread.  */
    loader_allocate_vbo(loader);

    /*  Next, build the OpenGL-dependent objects */
    model_t* model = model_new();
    backdrop_t* backdrop = backdrop_new();

    glfwShowWindow(window);
    log_trace("Showed window");

    OBJECT_ALLOC(instance);
    window_bind(window, instance);

    instance->backdrop = backdrop;
    instance->camera = camera;
    instance->model = model;
    instance->parent = parent;

    camera_update_proj(instance->camera);
    camera_reset_view(instance->camera);

    /*  At the very last moment, check on the loader */
    loader_finish(loader, instance->model, instance->camera);
    if (loader->state != LOADER_DONE) {
        instance->error = loader_error_string(loader->state);
    }
    loader_delete(loader);

    return instance;
}

void instance_delete(instance_t* instance) {
    OBJECT_DELETE_MEMBER(instance, model);
    OBJECT_DELETE_MEMBER(instance, camera);
    OBJECT_DELETE_MEMBER(instance, backdrop);
    OBJECT_DELETE_MEMBER(instance, window);
    free(instance);
}

/******************************************************************************/

void instance_cb_window_size(instance_t* instance, int width, int height)
{
    instance->camera->width = width;
    instance->camera->height = height;
    camera_update_proj(instance->camera);

    /*  Continue to render while the window is being resized
     *  (otherwise it ends up greyed out on Mac)  */
    instance_draw(instance, instance->parent->theme);
}

void instance_cb_mouse_pos(instance_t* instance, float xpos, float ypos) {
    camera_set_mouse_pos(instance->camera, xpos, ypos);
}

void instance_cb_mouse_click(instance_t* instance, int button,
                             int action, int mods)
{
    (void)mods;
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_1) {
            camera_begin_pan(instance->camera);
        } else if (button == GLFW_MOUSE_BUTTON_2) {
            camera_begin_rot(instance->camera);
        }
    } else {
        camera_end_drag(instance->camera);
    }
}
void instance_cb_mouse_scroll(instance_t* instance,
                              float xoffset, float yoffset)
{
    (void)xoffset;
    camera_zoom(instance->camera, yoffset);
}

void instance_cb_focus(instance_t* instance, bool focus)
{
    instance->focused = focus;
    if (focus) {
        app_set_front(instance->parent, instance);
    }
}

void instance_draw(instance_t* instance, theme_t* theme) {
    glfwMakeContextCurrent(instance->window);
    glClear(GL_DEPTH_BUFFER_BIT);
    backdrop_draw(instance->backdrop, theme);

    model_draw(instance->model, instance->camera, theme);
    glfwSwapBuffers(instance->window);
}
