#include "base.h"

struct instance_;
struct theme_;

typedef struct app_ {
    struct instance_** instances;
    unsigned instance_count;
    unsigned instances_size;

    struct theme_* theme;
    int draw_mode; // shaded or wireframe
    int draw_proj; // orthographic or perspective
} app_t;

/*  Calls instance_run on every instance */
bool app_run(app_t* app);

/*  Triggered from the UI */
struct instance_* app_open(app_t* app, const char* filename);

/*  Changes the view mode for the whole application */
void app_view_shaded(app_t* app);
void app_view_wireframe(app_t* app);
void app_view_orthographic(app_t* app);
void app_view_perspective(app_t* app);

/*  Moves a specific instance to the front of the list,
 *  which is used to determine the new focused window
 *  when the focused window is closed. */
void app_set_front(app_t* app, struct instance_* instance);

/*  Returns the instance that's currently focused */
struct instance_* app_get_front(app_t* app);
