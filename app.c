#include "app.h"
#include "instance.h"

instance_t* app_open(app_t* app, const char* filename) {
    instance_t* instance = instance_new(filename);

    /*  Search for an unused instance slot, which is marked
     *  by a NULL pointer in the instances array */
    unsigned i;
    for (i=0; i < app->num_instances; ++i) {
        if (app->instances[i] == NULL) {
            break;
        }
    }
    /*  Otherwise, extend the array by one */
    if (i == app->num_instances) {
        app->instances = (instance_t**)realloc(
                app->instances, sizeof(instance) * (i + 1));
        app->num_instances++;
    }
    app->instances[i] = instance;
    return instance;
}

void app_run(app_t* app) {
    for (unsigned i=0; i < app->num_instances; ++i) {
        if (app->instances[i] != NULL) {
            instance_run(app->instances[i]);
            if (glfwWindowShouldClose(app->instances[i]->window)) {
                instance_delete(app->instances[i]);
                app->instances[i] = NULL;
            }
        }
    }
}
