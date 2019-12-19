#include "base.h"

/*  Forward declaration of camera struct */
typedef struct camera_ camera_t;

typedef struct camera_uniforms_ {
    GLint proj;
    GLint view;
    GLint model;
} camera_uniforms_t;

typedef enum {
    CAMERA_PROJ_ORTHOGRAPHIC,
    CAMERA_PROJ_PERSPECTIVE
} camera_proj_t;

/*  Constructs a new heap-allocated camera */
camera_t* camera_new(float width, float height, camera_proj_t proj);
void camera_delete(camera_t* camera);

/*  Sets the camera width and height and updates proj matrix */
void camera_set_size(camera_t* camera, float width, float height);

/*  Sets the camera's model matrix.
 *  center must be an array of 3 floats */
void camera_set_model(camera_t* camera, float* center, float scale);

/*  Mouse handling functions */
void camera_begin_pan(camera_t* camera);
void camera_begin_rot(camera_t* camera);
void camera_end_drag(camera_t* camera);
void camera_zoom(camera_t* camera, float amount);

/*  Assigns the current mouse position, causing a rotation or
 *  pan if the button was already held down. */
void camera_set_mouse_pos(camera_t* camera, float x, float y);

/*  Schedules a camera animation to update projection */
void camera_anim_proj_perspective(camera_t* camera);
void camera_anim_proj_orthographic(camera_t* camera);

/*  Checks whether there is an animation running on the camera.
 *  If so, the values are updated.  If the animation is incomplete,
 *  returns true (so the caller should schedule a redraw). */
bool camera_check_anim(camera_t* camera);

/*  Looks up uniforms for camera binding */
camera_uniforms_t camera_get_uniforms(GLuint prog);

/*  Binds the camera matrices to the given uniforms */
void camera_bind(camera_t* camera, camera_uniforms_t u);
