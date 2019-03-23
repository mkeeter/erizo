#include "instance.h"
#include "backdrop.h"
#include "camera.h"
#include "loader.h"
#include "log.h"
#include "mat.h"
#include "model.h"
#include "object.h"
#include "window.h"

instance_t* instance_new(const char* filename) {
    /*  Kick the loader off in a separate thread */
    loader_t* loader = loader_new(filename);

    camera_t* camera = camera_new(500.0f, 500.0f);
    GLFWwindow* const window = window_new(camera->width, camera->height);

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
    instance_run(instance);
}

void instance_cb_mouse_pos(instance_t* instance, float xpos, float ypos) {
    camera_set_mouse_pos(instance->camera, xpos, ypos);
}

void instance_cb_mouse_click(instance_t* instance, int button, int action, int mods) {
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
void instance_cb_mouse_scroll(instance_t* instance, float xoffset, float yoffset) {
    camera_zoom(instance->camera, yoffset);
}

void instance_run(instance_t* instance) {
    glfwMakeContextCurrent(instance->window);
    glClear(GL_DEPTH_BUFFER_BIT);
    backdrop_draw(instance->backdrop);

    model_draw(instance->model, instance->camera);
    glfwSwapBuffers(instance->window);
}
