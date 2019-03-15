#include "platform.h"

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
} camera_t;

/*  Updates the proj matrix from width and height */
void camera_update_proj(camera_t* camera);

/*  Recalculates the view matrix */
void camera_update_view(camera_t* camera);
