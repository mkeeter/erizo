#include "base.h"

typedef struct icosphere_ {
    uint32_t num_vs;
    float (*vs)[3];

    uint32_t num_ts;
    uint32_t (*ts)[3];
} icosphere_t;

icosphere_t* icosphere_new(unsigned depth);
void icosphere_delete(icosphere_t* icosphere);

/*  Generates an STL buffer representing an icosphere
 *  Populates *size with the buffer size in bytes */
const char* icosphere_stl(unsigned depth, size_t* size);
