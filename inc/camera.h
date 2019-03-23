#include "platform.h"

typedef enum camera_mouse_ {
    CAMERA_IDLE,
    CAMERA_ROT,
    CAMERA_PAN,
} camera_mouse_t;

typedef struct camera_ {
    /*  Window parameters */
    int width;
    int height;

    /*  Camera positioning */
    float pitch;
    float yaw;
    float center[3];
    float scale;

    /*  Calculated matrices */
    float proj[4][4];
    float view[4][4];

    /* Matrix calculated in loader and stored in model */
    float model[4][4];

    /*  Mouse position and state tracking */
    camera_mouse_t state;
    float mouse_pos[2];
    float click_pos[2];
    float start[3]; /* Flexible drag data, depends on mode */
    float drag_mat[4][4];
} camera_t;

/*  Constructs a new heap-allocated camera */
camera_t* camera_new(float width, float height);
void camera_delete(camera_t* camera);

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
