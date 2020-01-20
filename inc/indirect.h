#include "base.h"

struct camera_;
struct model_;

typedef struct indirect_ indirect_t;

indirect_t* indirect_new(uint32_t width, uint32_t height);
void indirect_delete(indirect_t* indirect);
void indirect_resize(indirect_t* i, uint32_t width, uint32_t height);
void indirect_draw(indirect_t* draw, struct model_* model,
                   struct camera_* camera);
void indirect_blit(indirect_t* draw);
