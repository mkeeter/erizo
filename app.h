#include "platform.h"

struct loader_;
struct camera_;
struct model_;
struct backdrop_;

typedef struct app {
    struct backdrop_* backdrop;
    struct camera_* camera;
    struct loader_* loader;
    struct model_*  model;

    GLFWwindow* const window;
    int first;
} app_t;

void app_init(app_t* app);
void app_check_loader(app_t* app);
void app_draw(app_t* app);
