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
} app_state_t;

typedef struct app {
    app_state_t state;

    struct backdrop_* backdrop;
    struct camera_* camera;
    struct loader_* loader;
    struct model_*  model;

    GLFWwindow* const window;
} app_t;

void app_init(app_t* app);
void app_run(app_t* app);
