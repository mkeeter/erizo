#include "base.h"

struct instance_;

GLFWwindow* window_new(const char* filename, float width, float height);
void window_delete(GLFWwindow* window);
void window_bind(GLFWwindow* window, struct instance_* instance);
