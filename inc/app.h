#include "platform.h"

struct instance_;
struct theme_;

typedef struct app_ {
    struct instance_** instances;
    unsigned instance_count;
    unsigned instances_size;

    struct theme_* theme;
} app_t;

/*  Calls instance_run on every instance */
bool app_run(app_t* app);

/*  Triggered from the UI */
struct instance_* app_open(app_t* app, const char* filename);

/*  Moves a specific instance to the front of the list,
 *  which is used to determine the new focused window
 *  when the focused window is closed. */
void app_set_front(app_t* app, struct instance_* instance);
