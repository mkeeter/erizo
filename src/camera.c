#include "camera.h"
#include "draw.h"
#include "log.h"
#include "object.h"
#include "platform.h"
#include "shader.h"
#include "mat.h"

/*  Animation to change something about the camera */
typedef struct {
    enum {
        CAMERA_ANIM_LENS
    } type;

    vec3_t start_pos;
    vec3_t end_pos;

    int64_t start_time;
    int64_t total_time_ms;
} anim_t;

struct camera_ {
    /*  Window parameters */
    int width;
    int height;

    /*  Projection control (orthographic vs perspective) */
    float lens;

    /*  Camera positioning */
    float pitch;
    float yaw;
    vec3_t center;
    float scale;

    /*  Calculated matrices */
    mat4_t proj;
    mat4_t view;

    /* Matrix calculated in loader and stored in model */
    mat4_t model;

    /*  Mouse position and state tracking */
    enum { CAMERA_IDLE,
           CAMERA_ROT,
           CAMERA_PAN,
           CAMERA_ANIM,
    } state;

    float mouse_pos[2];
    float click_pos[2];
    vec3_t start; /* Flexible drag data, depends on mode */
    mat4_t drag_mat;

    anim_t* anim;
};

////////////////////////////////////////////////////////////////////////////////

/*  Updates the proj matrix from width and height */
static void camera_update_proj(camera_t* camera) {
    camera->proj = mat4_identity();
    const float aspect = (float)camera->width / (float)camera->height;
    if (aspect > 1) {
        camera->proj.m[0][0] = 1.0f / aspect;
    } else {
        camera->proj.m[1][1] = aspect;
    }
    camera->proj.m[2][2] = camera->scale / 2.0f;
    camera->proj.m[2][3] = camera->lens;
}

/*  Recalculates the view matrix */
static void camera_update_view(camera_t* camera) {
    camera->view = mat4_identity();

    /* Apply translation */
    camera->view = mat4_mul(camera->view, mat4_translation(camera->center));

    {   /* Apply the yaw rotation */
        const float c = cos(camera->yaw);
        const float s = sin(camera->yaw);
        const mat4_t y = {{{ c,   -s,   0.0f, 0.0f},
                           { s,    c,   0.0f, 0.0f},
                           {0.0f, 0.0f, 1.0f, 0.0f},
                           {0.0f, 0.0f, 0.0f, 1.0f}}};
        camera->view = mat4_mul(camera->view, y);
    }
    {   /* Apply the pitch rotation */
        const float c = cos(camera->pitch);
        const float s = sin(camera->pitch);
        const mat4_t p = {{{1.0f, 0.0f, 0.0f, 0.0f},
                           {0.0f,  c,   -s,   0.0f},
                           {0.0f,  s,    c,   0.0f},
                           {0.0f, 0.0f, 0.0f, 1.0f}}};
        camera->view = mat4_mul(camera->view, p);
    }

    {   /*  Apply the scaling */
        mat4_t s = mat4_scaling(1.0f / camera->scale);
        s.m[1][1] *= -1.0f;
        camera->view = mat4_mul(camera->view, s);
    }
}

////////////////////////////////////////////////////////////////////////////////

camera_t* camera_new(float width, float height, camera_proj_t proj) {
    OBJECT_ALLOC(camera);
    camera->width = width;
    camera->height = height;

    /*  Avoids a division by 0 during initial construction */
    camera->scale = 1.0f;

    switch (proj) {
        case CAMERA_PROJ_ORTHOGRAPHIC:  camera->lens = 0.0f; break;
        case CAMERA_PROJ_PERSPECTIVE:   camera->lens = 0.5f; break;
        default: log_error_and_abort("Invalid projection %i", proj);
    };

    /*  Initialize matrices to a sane state */
    camera_update_proj(camera);
    camera_update_view(camera);
    return camera;
}

void camera_delete(camera_t* camera) {
    free(camera->anim);
    free(camera);
}

void camera_set_size(camera_t* camera, float width, float height) {
    camera->width = width;
    camera->height = height;
    camera_update_proj(camera);
}

void camera_set_model(camera_t* camera, float* center, float scale) {
    mat4_t t = mat4_translation(*(vec3_t*)center);
    mat4_t s = mat4_scaling(1.0f / scale);
    camera->model = mat4_mul(t, s);
}

void camera_set_mouse_pos(camera_t* camera, float x, float y) {
    x = 2.0f * x / (camera->width) - 1.0f;
    y = 1.0f - 2.0f * y / (camera->height);
    camera->mouse_pos[0] = x;
    camera->mouse_pos[1] = y;

    const float dx = x - camera->click_pos[0];
    const float dy = y - camera->click_pos[1];

    switch (camera->state) {
        case CAMERA_IDLE:   /* Fallthrough */
        case CAMERA_ANIM:   break;
        case CAMERA_PAN: {
            vec3_t v = {{camera->click_pos[0], camera->click_pos[1], 0.0f}};
            v = mat4_apply(camera->drag_mat, v);
            vec3_t w = {{camera->mouse_pos[0], camera->mouse_pos[1], 0.0f}};
            w = mat4_apply(camera->drag_mat, w);
            for (unsigned i=0; i < 3; ++i) {
                camera->center.v[i] = camera->start.v[i] + v.v[i] - w.v[i];
            }
            camera_update_view(camera);
            break;
        }
        case CAMERA_ROT: {
            const float start_pitch = camera->start.v[0];
            const float start_yaw = camera->start.v[1];

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

static void camera_set_anim(camera_t* camera, anim_t* anim) {
    if (camera->anim) {
        log_warn("Triggered an animation while another was running; skipping");
        free(anim);
    } else {
        anim->start_time = platform_get_time();
        camera->anim = anim;
    }
}

static void camera_anim_lens(camera_t* camera, float target, int time_ms) {
    OBJECT_ALLOC(anim);
    anim->type = CAMERA_ANIM_LENS;
    anim->start_pos.v[0] = camera->lens;
    anim->end_pos.v[0] = target;
    anim->total_time_ms = time_ms;
    camera_set_anim(camera, anim);
}

void camera_anim_proj_perspective(camera_t* camera) {
    camera_anim_lens(camera, 0.5f, 100);
}

void camera_anim_proj_orthographic(camera_t* camera) {
    camera_anim_lens(camera, 0.0f, 100);
}

/*  Finds the inverse of the view + projection matrix.  This
 *  turns normalized mouse coordinates (in the +/- 1 range)
 *  into world coordinates. */
static mat4_t camera_vpi_mat(camera_t* camera) {
    mat4_t m = mat4_identity();
    m = mat4_mul(camera->proj, m);
    m = mat4_mul(camera->view, m);
    return mat4_inv(m);
}

void camera_begin_pan(camera_t* camera) {
    if (camera->state != CAMERA_IDLE) {
        log_warn("Cannot start panning in state %i", camera->state);
        return;
    }
    memcpy(camera->click_pos, camera->mouse_pos, sizeof(camera->mouse_pos));
    camera->start = camera->center;
    camera->drag_mat = camera_vpi_mat(camera);
    camera->state = CAMERA_PAN;
}

void camera_begin_rot(camera_t* camera) {
    if (camera->state != CAMERA_IDLE) {
        log_warn("Cannot start rotating in state %i", camera->state);
        return;
    }
    memcpy(camera->click_pos, camera->mouse_pos, sizeof(camera->mouse_pos));
    camera->start.v[0] = camera->pitch;
    camera->start.v[1] = camera->yaw;
    camera->state = CAMERA_ROT;
}

void camera_end_drag(camera_t* camera) {
    camera->state = CAMERA_IDLE;
}

void camera_zoom(camera_t* camera, float amount) {
    const vec3_t mouse = {{camera->mouse_pos[0], camera->mouse_pos[1], 0.0f}};

    mat4_t mat = camera_vpi_mat(camera);
    vec3_t before = mat4_apply(mat, mouse);

    camera->scale *= powf(1.01f, amount);
    camera_update_view(camera);
    camera_update_proj(camera);

    mat = camera_vpi_mat(camera);
    vec3_t after = mat4_apply(mat, mouse);

    for (unsigned i=0; i < 3; ++i) {
        camera->center.v[i] += before.v[i] - after.v[i];
    }
    camera_update_view(camera);
}

camera_uniforms_t camera_get_uniforms(GLuint prog) {
    camera_uniforms_t u;
    SHADER_GET_UNIFORM(proj);
    SHADER_GET_UNIFORM(view);
    SHADER_GET_UNIFORM(model);
    return u;
}

void camera_bind(camera_t* camera, camera_uniforms_t u) {
    glUniformMatrix4fv(u.proj,  1, GL_FALSE, (float*)&camera->proj);
    glUniformMatrix4fv(u.view,  1, GL_FALSE, (float*)&camera->view);
    glUniformMatrix4fv(u.model, 1, GL_FALSE, (float*)&camera->model);
}

bool camera_check_anim(camera_t* camera) {
    if (!camera->anim) {
        return false;
    }

    /*  Get the current time, for use in interpolation */
    const int64_t t = platform_get_time();
    if (t < camera->anim->start_time) {
        log_error_and_abort("Invalid time in animation");
    }

    /*  Calculate an interpolated value */
    float frac = (t - camera->anim->start_time) / camera->anim->total_time_ms
                 / 1000.0f;
    const bool done = (frac >= 1.0f);
    if (done) {
        frac = 1.0f;
    }
    vec3_t v;
    for (unsigned i=0; i < 3; ++i) {
        v.v[i] = camera->anim->start_pos.v[i] * (1.0f - frac) +
                 camera->anim->end_pos.v[i] * frac;
    }

    switch (camera->anim->type) {
        case CAMERA_ANIM_LENS: {
            camera->lens = v.v[0];
            camera_update_proj(camera);
       }
    }
    if (done) {
        free(camera->anim);
        camera->anim = NULL;
    }
    return !done;
}
