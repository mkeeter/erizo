#include "base.h"

typedef struct shaded_ shaded_t;
struct camera_;
struct model_;
struct theme_;

shaded_t* shaded_new();
void shaded_delete(shaded_t* shaded);
void shaded_draw(shaded_t* shaded, struct model_* model,
                 struct camera_* camera, struct theme_* theme);
