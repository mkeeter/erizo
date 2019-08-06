#include "base.h"

struct theme_;

typedef struct backdrop_ backdrop_t;

backdrop_t* backdrop_new();
void backdrop_delete(backdrop_t* backdrop);
void backdrop_draw(backdrop_t* backdrop, struct theme_* theme);
