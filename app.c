#include "app.h"
#include "instance.h"

instance_t* app_open(app_t* app, const char* filename) {
    instance_t* instance = instance_new(filename);
    app->instances = (instance_t**)realloc(
            app->instances, sizeof(instance) * (app->num_instances + 1));
    app->instances[app->num_instances] = instance;
    app->num_instances++;
    return instance;
}

void app_run(app_t* app) {
    for (unsigned i=0; i < app->num_instances; ++i) {
        instance_run(app->instances[i]);
    }
}
