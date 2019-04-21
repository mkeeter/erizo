#include "base.h"

struct ao_;
struct app_;
struct backdrop_;
struct camera_;
struct model_;
struct theme_;

typedef struct instance_ {
    struct backdrop_* backdrop;
    struct camera_* camera;
    struct model_*  model;

    struct ao_* ao;

    const char* error; // Error string from the loader
    struct app_* parent;

    bool focused;

    GLFWwindow* window;
} instance_t;

instance_t* instance_new(struct app_* parent, const char* filepath);
void instance_delete(instance_t* instance);
void instance_draw(instance_t* instance, struct theme_* theme);

/*  Callbacks */
void instance_cb_window_size(instance_t* instance, int width, int height);
void instance_cb_mouse_pos(instance_t* instance, float xpos, float ypos);
void instance_cb_mouse_click(instance_t* instance, int button, int action, int mods);
void instance_cb_mouse_scroll(instance_t* instance, float xoffset, float yoffset);
void instance_cb_focus(instance_t* instance, bool focus);
