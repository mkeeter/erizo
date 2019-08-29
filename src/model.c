#include "camera.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "theme.h"

model_t* model_new() {
    OBJECT_ALLOC(model);
    model->tri_count = 0;
    model->vbo = 0;
    model->ibo = 0;
    glGenVertexArrays(1, &model->vao);
    log_trace("Initialized model");
    return model;
}

void model_delete(model_t* model) {
    glDeleteBuffers(1, &model->vbo);
    glDeleteBuffers(1, &model->ibo);
    glDeleteVertexArrays(1, &model->vbo);
    free(model);
}
