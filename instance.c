#include "instance.h"
#include "backdrop.h"
#include "camera.h"
#include "loader.h"
#include "log.h"
#include "mat.h"
#include "model.h"
#include "window.h"

static int first = 1;

instance_t* instance_new(const char* filename) {
    loader_t* loader = loader_new(filename);

    if (first) {
        if (!glfwInit()) {
            log_error_and_abort("Failed to initialize glfw");
        }
        glfwWindowHint(GLFW_SAMPLES, 8);    /* multisampling! */
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    camera_t* camera = (camera_t*)calloc(sizeof(camera_t), 1);
    camera->width = 500.0f;
    camera->height = 500.0f;

    GLFWwindow* const window = glfwCreateWindow(
            camera->width, camera->height,
            "hedgehog", NULL, NULL);
    if (!window) {
        log_error_and_abort("Failed to create window");
    } else {
        log_trace("Created window");
    }

    glfwMakeContextCurrent(window);
    log_trace("Made context current");

    if (first) {
        const GLenum glew_err = glewInit();
        if (GLEW_OK != glew_err) {
            log_error_and_abort("GLEW initialization failed: %s\n",
                                glewGetErrorString(glew_err));
        }
        log_trace("Initialized GLEW");
    }

    /*  Highest priority once OpenGL is running: allocate the VBO
     *  and pass it to the loader thread.  */
    loader_allocate_vbo(loader);

    /*  Next, build the OpenGL-dependent objects */
    model_t* model = (model_t*)calloc(sizeof(model_t), 1);
    model_init(model);

    backdrop_t* backdrop = (backdrop_t*)calloc(sizeof(backdrop_t), 1);
    backdrop_init(backdrop);

    glfwShowWindow(window);
    log_trace("Showed window");

    instance_t* instance = (instance_t*)calloc(sizeof(instance_t), 1);
    window_set_callbacks(window, instance);

    instance->backdrop = backdrop;
    instance->camera = camera;
    instance->loader = loader;
    instance->model = model;
    instance->window = window;

    camera_update_proj(instance->camera);
    camera_update_view(instance->camera);

    if (first) {
        glClearDepth(1.0);
        first = 0;
    }
    return instance;
}

/******************************************************************************/

void instance_cb_keypress(instance_t* instance, int key, int scancode,
                     int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(instance->window, 1);
    }
}

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
}

void instance_run(instance_t* instance) {
    glfwMakeContextCurrent(instance->window);
    glClear(GL_DEPTH_BUFFER_BIT);
    backdrop_draw(instance->backdrop);

    /*  At the very last moment, check on the loader */
    if (instance->loader) {
        loader_wait(instance->loader, LOADER_DONE);
        loader_finish(instance->loader, instance->model, instance->camera);
    }
    model_draw(instance->model, instance->camera);
    glfwSwapBuffers(instance->window);

    /*  Print a timing message on first load */
    if (instance->loader) {
        log_info("Load complete");
        loader_reset(instance->loader);
        instance->loader = NULL;
    }
}
