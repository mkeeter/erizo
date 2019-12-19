#include "base.h"

/* Forward declarations */
struct camera_;
struct model_;
struct theme_;

typedef struct draw_ draw_t;

/*  Constructs a new draw object.  gs is optional and may be NULL */
draw_t* draw_new(const char* vs, const char* gs, const char* fs);
void draw_delete(draw_t* d);

void draw(draw_t* draw, struct model_* model,
          struct camera_* camera, struct theme_* theme);
