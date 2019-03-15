#include "platform.h"

typedef enum app_mouse_ {
    APP_MOUSE_UP,
    APP_MOUSE_ROT,
    APP_MOUSE_PAN,
} camera_mouse_t;

typedef struct camera_ {
    /*  Window parameters */
    int width;
    int height;

    /*  Camera positioning */
    float pitch;
    float yaw;
    float center[3];

    /*  Calculated matrices */
    float proj[4][4];
    float view[4][4];

    /* Matrix calculated in loader and stored in model */
    float model[4][4];

    /*  Mouse position and state tracking */
    camera_mouse_t state;
    float mouse_pos[2];
    float click_pos[2];
} camera_t;

/*  Updates the proj matrix from width and height */
void camera_update_proj(camera_t* camera);

/*  Recalculates the view matrix */
void camera_update_view(camera_t* camera);

/*  Converts a position in mouse coordinates (0 to window height / width)
 *  into world coordinates, assigning to the given floats */
void camera_mouse_to_world(camera_t* camera, float* x, float* y);
