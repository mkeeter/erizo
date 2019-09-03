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
#include "shaded.h"
#include "theme.h"
#include "window.h"
#include "wireframe.h"

instance_t* instance_new(app_t* parent, const char* filepath) {
    /*  Kick the loader off in a separate thread */
    loader_t* loader = loader_new(filepath);

    const float width = 500;
    const float height = 500;
    const char* filename = platform_filename(filepath);
    GLFWwindow* window = window_new(filename, width, height);

    /*  Highest priority once OpenGL is running: allocate the VBO
     *  and pass it to the loader thread.  */
    loader_allocate_vbo(loader);

    glfwShowWindow(window);
    log_trace("Showed window");

    OBJECT_ALLOC(instance);
    instance->parent = parent;

    /*  Next, build the OpenGL-dependent objects */
    instance->backdrop = backdrop_new();
    instance->camera = camera_new(width, height);
    instance->model = model_new();
    instance->shaded = shaded_new();
    instance->wireframe = wireframe_new();

    camera_update_proj(instance->camera);
    camera_reset_view(instance->camera);

    /*  At the very last moment, check on the loader */
    loader_finish(loader, instance->model, instance->camera);

    /*  This needs to happen after setting up the instance, because
     *  on Windows, the window size callback is invoked when we add
     *  the menu, which requires the camera to be populated. */
    window_bind(window, instance);

    /*  Sets the error string, or NULL if there was no error. */
    instance->error = loader_error_string(loader);

    /*  Clean up and return */
    loader_delete(loader);
    return instance;
}

void instance_delete(instance_t* instance) {
    OBJECT_DELETE_MEMBER(instance, backdrop);
    OBJECT_DELETE_MEMBER(instance, camera);
    OBJECT_DELETE_MEMBER(instance, model);
    OBJECT_DELETE_MEMBER(instance, shaded);
    OBJECT_DELETE_MEMBER(instance, window);
    free(instance);
}

/******************************************************************************/

void instance_cb_window_size(instance_t* instance, int width, int height)
{
    /*  Update camera size (and recalculate projection matrix) */
    camera_set_size(instance->camera, width, height);

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

    shaded_draw(instance->shaded, instance->model, instance->camera, theme);
    glfwSwapBuffers(instance->window);
}
