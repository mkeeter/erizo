#ifndef PLATFORM
#define PLATFORM

#include <stdint.h>

const char* platform_mmap(const char* filename);
void platform_get_time(int64_t* sec, int32_t* usec);

#endif
