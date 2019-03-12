struct loader_;
struct camera_;
struct model_;
struct backdrop_;

typedef struct app {
    struct backdrop_* const backdrop;
    struct camera_* const camera;
    struct loader_* const loader;
    struct model_*  const model;
} app_t;
