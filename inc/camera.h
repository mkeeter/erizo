#include "base.h"

/*  Forward declaration of camera struct */
typedef struct camera_ camera_t;
struct draw_;

/*  Constructs a new heap-allocated camera */
camera_t* camera_new(float width, float height);
void camera_delete(camera_t* camera);

/*  Sets the camera width and height and updates proj matrix */
void camera_set_size(camera_t* camera, float width, float height);

/*  Used when binding uniforms */
float* camera_model_mat(camera_t* camera);

/*  Updates the proj matrix from width and height */
void camera_update_proj(camera_t* camera);

/*  Recalculates the view matrix */
void camera_update_view(camera_t* camera);

/*  Resets center and scale, then recalculates the view matrix */
void camera_reset_view(camera_t* camera);

/*  Mouse handling functions */
void camera_begin_pan(camera_t* camera);
void camera_begin_rot(camera_t* camera);
void camera_end_drag(camera_t* camera);
void camera_zoom(camera_t* camera, float amount);

/*  Assigns the current mouse position, causing a rotation or
 *  pan if the button was already held down. */
void camera_set_mouse_pos(camera_t* camera, float x, float y);

/*  Binds the camera matrices to the draw target */
void camera_bind(camera_t* camera, struct draw_* draw);
