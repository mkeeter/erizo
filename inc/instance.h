#include "base.h"

struct app_;
struct backdrop_;
struct camera_;
struct indirect_;
struct model_;
struct theme_;

typedef struct instance_ {
    struct backdrop_* backdrop;
    struct camera_* camera;
    struct model_* model;

    // Different rendering modes
    struct draw_* shaded;
    struct draw_* wireframe;

    struct indirect_* indirect;

    enum {
        DRAW_SHADED,
        DRAW_WIREFRAME
    } draw_mode;

    const char* error; // Error string from the loader
    struct app_* parent;

    bool focused;

    GLFWwindow* window;
} instance_t;

instance_t* instance_new(struct app_* parent, const char* filepath, int proj);
void instance_delete(instance_t* instance);

/*  Draws an instance
 *
 *  Returns true if the main loop should schedule a redraw immediately
 *  (e.g. because there's an animation running in this instance). */
bool instance_draw(instance_t* instance, struct theme_* theme);

void instance_view_shaded(instance_t* instance);
void instance_view_wireframe(instance_t* instance);
void instance_view_orthographic(instance_t* instance);
void instance_view_perspective(instance_t* instance);

/*  Callbacks */
void instance_cb_window_size(instance_t* instance, int width, int height);
void instance_cb_mouse_pos(instance_t* instance, float xpos, float ypos);
void instance_cb_mouse_click(instance_t* instance, int button, int action, int mods);
void instance_cb_mouse_scroll(instance_t* instance, float xoffset, float yoffset);
void instance_cb_focus(instance_t* instance, bool focus);
