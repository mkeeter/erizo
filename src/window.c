#include "app.h"
#include "instance.h"
#include "log.h"
#include "platform.h"
#include "window.h"

static void cb_window_size(GLFWwindow* window, int width, int height) {
    instance_t* instance = (instance_t*)glfwGetWindowUserPointer(window);
    instance_cb_window_size(instance, width, height);
}

static void cb_mouse_pos(GLFWwindow* window, double x, double y) {
    instance_t* instance = (instance_t*)glfwGetWindowUserPointer(window);
    instance_cb_mouse_pos(instance, x, y);
}

static void cb_mouse_scroll(GLFWwindow* window, double dx, double dy) {
    instance_t* instance = (instance_t*)glfwGetWindowUserPointer(window);
    instance_cb_mouse_scroll(instance, dx, dy);
}

static void cb_mouse_click(GLFWwindow* window, int button,
                           int action, int mods)
{
    instance_t* instance = (instance_t*)glfwGetWindowUserPointer(window);
    instance_cb_mouse_click(instance, button, action, mods);
}

static void cb_drop(GLFWwindow* window, int count, const char** paths)
{
    instance_t* instance = (instance_t*)glfwGetWindowUserPointer(window);
    app_t* app = instance->parent;
    for (int i=0; i < count;  i++) {
        app_open(app, paths[i]);
    }
}

static void cb_focus(GLFWwindow* window, int focus)
{
    instance_t* instance = (instance_t*)glfwGetWindowUserPointer(window);
    instance_cb_focus(instance, focus);
}

static void cb_close(GLFWwindow* window)
{
    //  Kick the main loop, so that it exits if all windows are closed
    (void)window;
    glfwPostEmptyEvent();
}

////////////////////////////////////////////////////////////////////////////////

void window_bind(GLFWwindow* window, instance_t* instance) {
    instance->window = window;
    glfwSetWindowUserPointer(window, instance);

    glfwSetWindowSizeCallback(window, cb_window_size);

    glfwSetCursorPosCallback(window, cb_mouse_pos);
    glfwSetScrollCallback(window, cb_mouse_scroll);
    glfwSetMouseButtonCallback(window, cb_mouse_click);
    glfwSetDropCallback(window, cb_drop);
    glfwSetWindowFocusCallback(window, cb_focus);
    glfwSetWindowCloseCallback(window, cb_close);

    platform_window_bind(window);
}

GLFWwindow* window_new(const char* filename, float width, float height) {
    static bool first = true;

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

    GLFWwindow* const window = glfwCreateWindow(
            width, height, filename, NULL, NULL);
    if (!window) {
        const char* err;
        glfwGetError(&err);
        log_error_and_abort("Failed to create window: %s", err);
    } else {
        log_trace("Created window");
    }

    glfwMakeContextCurrent(window);
    log_trace("Made context current");

    if (first) {
        const GLenum glew_err = glewInit();
        if (GLEW_OK != glew_err) {
            log_error_and_abort("GLEW initialization failed: %s",
                                glewGetErrorString(glew_err));
        }
        log_trace("Initialized GLEW");
        glClearDepth(1.0);
        first = false;
    }
    return window;
}

void window_delete(GLFWwindow* window) {
    glfwDestroyWindow(window);
}
