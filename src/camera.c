#include "platform.h"

#include "camera.h"
#include "object.h"
#include "mat.h"

camera_t* camera_new(float width, float height) {
    OBJECT_ALLOC(camera);
    camera->width = width;
    camera->height = height;

    /*  Avoids a division by 0 during initial construction */
    camera->scale = 1.0f;
    return camera;
}

void camera_delete(camera_t* camera) {
    free(camera);
}

void camera_update_proj(camera_t* camera) {
    mat4_identity(camera->proj);
    const float aspect = (float)camera->width / (float)camera->height;
    if (aspect > 1) {
        camera->proj[0][0] /= aspect;
    } else {
        camera->proj[1][1] *= aspect;
    }
}

void camera_reset_view(camera_t* camera) {
    camera->scale = 1.0f;
    memset(camera->center, 0, sizeof(camera->center));
    camera_update_view(camera);
}

void camera_update_view(camera_t* camera) {
    mat4_identity(camera->view);

    {   /* Apply translation */
        float t[4][4];
        mat4_translation(camera->center, t);
        mat4_mul(camera->view, t, camera->view);
    }

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

    {   /*  Apply the scaling */
        float s[4][4];
        mat4_scaling(1.0f / camera->scale, s);
        s[1][1] *= -1.0f;
        mat4_mul(camera->view, s, camera->view);
    }
}

void camera_set_mouse_pos(camera_t* camera, float x, float y) {

    x = 2.0f * x / (camera->width) - 1.0f;
    y = 1.0f - 2.0f * y / (camera->height);
    camera->mouse_pos[0] = x;
    camera->mouse_pos[1] = y;

    const float dx = x - camera->click_pos[0];
    const float dy = y - camera->click_pos[1];

    switch (camera->state) {
        case CAMERA_IDLE:  break;
        case CAMERA_PAN: {
            float v[3] = {camera->click_pos[0], camera->click_pos[1], 0.0f};
            mat4_apply(camera->drag_mat, v, v);
            float w[3] = {camera->mouse_pos[0], camera->mouse_pos[1], 0.0f};
            mat4_apply(camera->drag_mat, w, w);
            for (unsigned i=0; i < 3; ++i) {
                camera->center[i] = camera->start[i] + v[i] - w[i];
            }
            camera_update_view(camera);
            break;
        }
        case CAMERA_ROT: {
            const float start_pitch = camera->start[0];
            const float start_yaw = camera->start[1];

            /*  Update pitch and clamp values */
            camera->pitch = start_pitch + dy * 2.0f;
            if (camera->pitch < -M_PI) {
                camera->pitch = -M_PI;
            } else if (camera->pitch > 0.0f) {
                camera->pitch = 0.0f;
            }

            /*  Update yaw and keep it under 360 degrees */
            camera->yaw = start_yaw - dx * 2.0f;
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

/*  Finds the inverse of the view + projection matrix.  This
 *  turns normalized mouse coordinates (in the +/- 1 range)
 *  into world coordinates. */
static void camera_vpi_mat(camera_t* camera, float m[4][4]) {
    mat4_identity(m);
    mat4_mul(camera->proj, m, m);
    mat4_mul(camera->view, m, m);
    mat4_inv(m, m);
}

void camera_begin_pan(camera_t* camera) {
    memcpy(camera->click_pos, camera->mouse_pos, sizeof(camera->mouse_pos));
    memcpy(camera->start, camera->center, sizeof(camera->center));

    camera_vpi_mat(camera, camera->drag_mat);
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

void camera_zoom(camera_t* camera, float amount) {
    const float mouse[3] = {camera->mouse_pos[0], camera->mouse_pos[1], 0.0f};

    float mat[4][4];
    camera_vpi_mat(camera, mat);
    float before[3];
    mat4_apply(mat, mouse, before);

    camera->scale *= powf(1.01f, amount);
    camera_update_view(camera);

    camera_vpi_mat(camera, mat);
    float after[3];
    mat4_apply(mat, mouse, after);

    for (unsigned i=0; i < 3; ++i) {
        camera->center[i] += before[i] - after[i];
    }
    camera_update_view(camera);
}
