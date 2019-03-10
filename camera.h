#ifndef CAMERA_H
#define CAMERA_H

#include "platform.h"

typedef struct camera_ {
    float proj[16];
} camera_t;

/*
 *  Updates the proj matrix from width and height
 */
void camera_update_proj(uint32_t width, uint32_t height);

#endif
