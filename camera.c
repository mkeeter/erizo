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
        const float y[4][4] = {{ c,   -s,   0.0f, 0.0f},
                               { s,    c,   0.0f, 0.0f},
                               {0.0f, 0.0f, 1.0f, 0.0f},
                               {0.0f, 0.0f, 0.0f, 1.0f}};
        mat4_mul(camera->view, y, camera->view);
    }
    {   /* Apply the pitch rotation */
        const float c = cos(camera->pitch);
        const float s = sin(camera->pitch);
        const float p[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                               {0.0f,  c,   -s,   0.0f},
                               {0.0f,  s,    c,   0.0f},
                               {0.0f, 0.0f, 0.0f, 1.0f}};
        mat4_mul(camera->view, p, camera->view);
    }

    {
        float s[4][4];
        mat4_scaling(1.0f / camera->scale, s);
        mat4_mul(camera->view, s, camera->view);
    }
}

void camera_set_mouse_pos(camera_t* camera, float x, float y) {
    camera->mouse_pos[0] = x;
    camera->mouse_pos[1] = y;

    const float dx = x - camera->click_pos[0];
    const float dy = y - camera->click_pos[1];

    switch (camera->state) {
        case CAMERA_IDLE:  break;
        case CAMERA_PAN: {
            /* TODO */
            break;
        }
        case CAMERA_ROT: {
            const float start_pitch = camera->start[0];
            const float start_yaw = camera->start[1];

            /*  Update pitch and clamp values */
            camera->pitch = start_pitch + dy / 100.0f;
            if (camera->pitch < -M_PI) {
                camera->pitch = -M_PI;
            } else if (camera->pitch > M_PI) {
                camera->pitch = M_PI;
            }

            /*  Update yaw and keep it under 360 degrees */
            camera->yaw = start_yaw - dx / 100.0f;
            while (camera->yaw < 0.0f) {
                camera->yaw += 2.0f * M_PI;
            }
            while (camera->yaw > 2.0f * M_PI) {
                camera->yaw -= 2.0f * M_PI;
            }

            /*  Rebuild view matrix with new values */
            camera_update_view(camera);
            break;
         }
    }
}

void camera_begin_pan(camera_t* camera) {
    memcpy(camera->click_pos, camera->mouse_pos, sizeof(camera->mouse_pos));
    memcpy(camera->start, camera->center, sizeof(camera->center));
    camera->state = CAMERA_PAN;
}

void camera_begin_rot(camera_t* camera) {
    memcpy(camera->click_pos, camera->mouse_pos, sizeof(camera->mouse_pos));
    camera->start[0] = camera->pitch;
    camera->start[1] = camera->yaw;
    camera->state = CAMERA_ROT;
}

void camera_end_drag(camera_t* camera) {
    camera->state = CAMERA_IDLE;
}
