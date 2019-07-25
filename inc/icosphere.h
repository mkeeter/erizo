#include "base.h"

/*  Forward declaration */
typedef struct icosphere_ icosphere_t;

icosphere_t* icosphere_new(unsigned depth);
void icosphere_delete(icosphere_t* icosphere);

/*  Generates an STL buffer representing an icosphere
 *  Populates *size with the buffer size in bytes */
const char* icosphere_stl(unsigned depth, size_t* size);
