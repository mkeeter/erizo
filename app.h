#include "platform.h"

struct loader_;
struct camera_;
struct model_;
struct backdrop_;

typedef enum app_state_ {
    APP_PRELOAD,
    APP_PREDRAW,
    APP_LOAD,
    APP_RUNNING,
    APP_QUIT
} app_state_t;

typedef struct app_ {
    app_state_t state;

    struct backdrop_* backdrop;
    struct camera_* camera;
    struct loader_* loader;
    struct model_*  model;

    GLFWwindow* window;
} app_t;

void app_init(app_t* app);
void app_run(app_t* app);

/*  Triggered from the UI */
void app_open(app_t* app, const char* filename);

/*  Callbacks */
void app_cb_window_size(app_t* app, int width, int height);
void app_cb_keypress(app_t* app, int key, int scancode, int action, int mods);
void app_cb_mouse_pos(app_t* app, float xpos, float ypos);
void app_cb_mouse_click(app_t* app, int button, int action, int mods);
void app_cb_mouse_scroll(app_t* app, float xoffset, float yoffset);
