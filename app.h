#ifndef APP_H
#define APP_H

struct loader_;
struct camera_;

typedef struct app {
    struct camera_* const camera;
} app_t;

#endif
