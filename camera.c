#include "camera.h"

void camera_update_proj(camera_t* camera, uint32_t width, uint32_t height) {
    camera->width = width;
    camera->height = height;

    memset(camera->proj, 0, sizeof(camera->proj));
    for (unsigned i=0; i < 4; ++i) {
        camera->proj[i + i * 4] = 1.0f;
    }
}
