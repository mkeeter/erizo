#include "base.h"

typedef struct wireframe_ wireframe_t;
struct camera_;
struct model_;
struct theme_;

wireframe_t* wireframe_new();
void wireframe_delete(wireframe_t* wireframe);
void wireframe_draw(wireframe_t* wireframe, struct model_* model,
                    struct camera_* camera, struct theme_* theme);

