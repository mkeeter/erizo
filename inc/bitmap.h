#include "base.h"

void bitmap_write_header(FILE* f, unsigned width, unsigned height);
void bitmap_write_depth(FILE* f, unsigned w, unsigned h, uint32_t* data);
