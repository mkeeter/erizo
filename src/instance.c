#include "app.h"
#include "backdrop.h"
#include "camera.h"
#include "draw.h"
#include "instance.h"
#include "indirect.h"
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

instance_t* instance_new(app_t* parent, const char* filepath, int proj) {
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
    instance->camera = camera_new(width, height, proj);
    instance->model = model_new();
    instance->shaded = shaded_new();
    instance->wireframe = wireframe_new();
    instance->draw_mode = DRAW_SHADED;

    instance->indirect = indirect_new(width, height);

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
    draw_delete(instance->shaded);
    draw_delete(instance->wireframe);
    OBJECT_DELETE_MEMBER(instance, window);
    OBJECT_DELETE_MEMBER(instance, indirect);
    free(instance);
}

static void instance_set_view(instance_t* instance, int draw_mode) {
    instance->draw_mode = draw_mode;
    glfwPostEmptyEvent();
}

void instance_view_shaded(instance_t* instance) {
    instance_set_view(instance, DRAW_SHADED);
}

void instance_view_wireframe(instance_t* instance) {
    instance_set_view(instance, DRAW_WIREFRAME);
}

void instance_view_orthographic(instance_t* instance) {
    camera_anim_proj_orthographic(instance->camera);
}

void instance_view_perspective(instance_t* instance) {
    camera_anim_proj_perspective(instance->camera);
}

/******************************************************************************/

void instance_cb_window_size(instance_t* instance, int width, int height)
{
    /*  Update camera size (and recalculate projection matrix) */
    camera_set_size(instance->camera, width, height);

    /*  Resize buffers for indirect rendering */
    indirect_resize(instance->indirect, width, height);

#ifdef PLATFORM_DARWIN
    /*  Continue to render while the window is being resized
     *  (otherwise it ends up greyed out on Mac)  */
    instance_draw(instance, instance->parent->theme);
#endif

#ifdef PLATFORM_WIN32
    /*  Otherwise, it misbehaves in a high-DPI environment */
    glfwMakeContextCurrent(instance->window);
    glViewport(0, 0, width, height);
#endif
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

bool instance_draw(instance_t* instance, theme_t* theme) {
    const bool needs_redraw = camera_check_anim(instance->camera);

    glfwMakeContextCurrent(instance->window);
    indirect_draw(instance->indirect, instance->model, instance->camera);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    backdrop_draw(instance->backdrop, theme);

    switch (instance->draw_mode) {
        case DRAW_SHADED:
            draw(instance->shaded, instance->model, instance->camera, theme);
            break;
        case DRAW_WIREFRAME:
            draw(instance->wireframe, instance->model, instance->camera, theme);
            break;
    }

    glfwSwapBuffers(instance->window);
    return needs_redraw;
}
