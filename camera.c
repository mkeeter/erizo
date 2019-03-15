#include <math.h>
#include "camera.h"
#include "mat.h"

void camera_update_proj(camera_t* camera) {
    mat4_identity(camera->proj);
    const float aspect = (float)camera->width / (float)camera->height;
    if (aspect > 1) {
        camera->proj[0][0] /= aspect;
    } else {
        camera->proj[1][1] *= aspect;
    }
}

void camera_update_view(camera_t* camera) {
    mat4_identity(camera->view);

    {   /* Apply the yaw rotation */
        const float c = cos(camera->yaw);
        const float s = sin(camera->yaw);
        const float yaw[4][4] = {{ c,   -s,   0.0f, 0.0f},
                                 { s,    c,   0.0f, 0.0f},
                                 {0.0f, 0.0f, 1.0f, 0.0f},
                                 {0.0f, 0.0f, 0.0f, 1.0f}};
        mat4_mul(camera->view, yaw, camera->view);
    }
}

void camera_mouse_to_world(camera_t* camera, float* x, float* y) {
    float out[4][4];
    mat4_identity(out);
    mat4_mul(camera->model, out, out);
    mat4_mul(camera->proj, out, out);
    mat4_mul(camera->view, out, out);
    mat4_inv(out, out);

    float pos[3] = { 2.0f * (*x) / (camera->width)  - 1.0f,
                     2.0f * (*y) / (camera->height) - 1.0f,
                     0.0f};
    mat4_apply(out, pos, pos);

    *x = pos[0];
    *y = pos[1];
}

