#include "platform.h"

struct instance_;
struct theme_;

typedef struct app_ {
    struct instance_** instances;
    unsigned num_instances;
    struct theme_* theme;
} app_t;

/*  Calls instance_run on every instance */
bool app_run(app_t* app);

/*  Triggered from the UI */
struct instance_* app_open(app_t* app, const char* filename);
