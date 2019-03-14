#include "camera.h"

void camera_update_proj(camera_t* camera, int width, int height) {
    camera->width = width;
    camera->height = height;

    memset(camera->proj, 0, sizeof(camera->proj));
    for (unsigned i=0; i < 4; ++i) {
        camera->proj[i][i] = 1.0f;
    }
    const float aspect = (float)width / (float)height;
    if (aspect > 1) {
        camera->proj[0][0] /= aspect;
    } else {
        camera->proj[1][1] *= aspect;
    }
}
