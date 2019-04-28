#include "base.h"

void bitmap_write_header(FILE* f, unsigned width, unsigned height);
void bitmap_write_depth(FILE* f, unsigned w, unsigned h, const uint32_t* data);
void bitmap_write_rays(FILE* f, unsigned w, unsigned h,
                       const float (*data)[4], uint32_t rays);
