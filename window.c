#include "instance.h"
#include "window.h"

static void cb_key(GLFWwindow* window, int key, int scancode,
                         int action, int mods)
{
    instance_t* instance = (instance_t*)glfwGetWindowUserPointer(window);
    instance_cb_keypress(instance, key, scancode, action, mods);
}

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

void window_set_callbacks(GLFWwindow* window, instance_t* instance) {
    glfwSetWindowUserPointer(window, instance);

    glfwSetKeyCallback(window, cb_key);
    glfwSetWindowSizeCallback(window, cb_window_size);

    glfwSetCursorPosCallback(window, cb_mouse_pos);
    glfwSetScrollCallback(window, cb_mouse_scroll);
    glfwSetMouseButtonCallback(window, cb_mouse_click);
}
