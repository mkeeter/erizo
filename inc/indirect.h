#include "base.h"

typedef struct indirect_ indirect_t;

indirect_t* indirect_new(uint32_t width, uint32_t height);
void indirect_resize(indirect_t* i, uint32_t width, uint32_t height);
