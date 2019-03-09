#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

const char* platform_mmap(const char* filename) {
    int stl_fd = open(filename, O_RDONLY);
    struct stat s;
    fstat(stl_fd, &s);
    size_t size = s.st_size;

    return mmap(0, size, PROT_READ, MAP_PRIVATE, stl_fd, 0);
}

void platform_get_time(int64_t* sec, int32_t* usec) {
    static struct timeval t;
    gettimeofday(&t, NULL);

    *sec = t.tv_sec;
    *usec = t.tv_usec;
}
