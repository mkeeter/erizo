#include "platform.h"

typedef struct camera_ {
    int width;
    int height;
    float proj[16];
} camera_t;

/*  Updates the proj matrix from width and height */
void camera_update_proj(camera_t* camera, int width, int height);
