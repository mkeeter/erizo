#include "app.h"
#include "window.h"

static void cb_key(GLFWwindow* window, int key, int scancode,
                         int action, int mods)
{
    app_t* app = (app_t*)glfwGetWindowUserPointer(window);
    app_cb_keypress(app, key, scancode, action, mods);
}

static void cb_window_size(GLFWwindow* window, int width, int height) {
    app_t* app = (app_t*)glfwGetWindowUserPointer(window);
    app_cb_window_size(app, width, height);
}

static void cb_mouse_pos(GLFWwindow* window, double x, double y) {
    app_t* app = (app_t*)glfwGetWindowUserPointer(window);
    app_cb_mouse_pos(app, x, y);
}

static void cb_mouse_scroll(GLFWwindow* window, double dx, double dy) {
    app_t* app = (app_t*)glfwGetWindowUserPointer(window);
    app_cb_mouse_scroll(app, dx, dy);
}

static void cb_mouse_click(GLFWwindow* window, int button,
                           int action, int mods)
{
    app_t* app = (app_t*)glfwGetWindowUserPointer(window);
    app_cb_mouse_click(app, button, action, mods);
}

void window_set_callbacks(GLFWwindow* window, app_t* app) {
    glfwSetWindowUserPointer(window, app);

    glfwSetKeyCallback(window, cb_key);
    glfwSetWindowSizeCallback(window, cb_window_size);

    glfwSetCursorPosCallback(window, cb_mouse_pos);
    glfwSetScrollCallback(window, cb_mouse_scroll);
    glfwSetMouseButtonCallback(window, cb_mouse_click);
}
