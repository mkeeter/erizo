#include "platform.h"

typedef struct camera_ {
    uint32_t width;
    uint32_t height;
    float proj[16];
} camera_t;

/*  Updates the proj matrix from width and height */
void camera_update_proj(camera_t* camera, uint32_t width, uint32_t height);
