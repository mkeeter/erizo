#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "platform.h"

const char* platform_mmap(const char* filename, size_t* size) {
    int stl_fd = open(filename, O_RDONLY);
    struct stat s;
    fstat(stl_fd, &s);
    *size = s.st_size;

    return mmap(0, *size, PROT_READ, MAP_PRIVATE, stl_fd, 0);
}

void platform_munmap(const char* data, size_t size) {
    munmap((void*)data, size);
}

void platform_get_time(int64_t* sec, int32_t* usec) {
    static struct timeval t;
    gettimeofday(&t, NULL);

    *sec = t.tv_sec;
    *usec = t.tv_usec;
}

void platform_set_terminal_color(platform_terminal_color_t c) {
    printf("\x1b[3%im", c - TERM_COLOR_BLACK);
}

void platform_clear_terminal_color() {
    printf("\x1b[0m");
}

int platform_mutex_init(platform_mutex_t* mutex) {
    return pthread_mutex_init(&mutex->data, NULL);
}

int platform_cond_init(platform_cond_t* cond) {
    return pthread_cond_init(&cond->data, NULL);
}

int platform_mutex_lock(platform_mutex_t* mutex) {
    return pthread_mutex_lock(&mutex->data);
}

int platform_mutex_unlock(platform_mutex_t* mutex) {
    return pthread_mutex_unlock(&mutex->data);
}

int platform_cond_wait(platform_cond_t* cond, platform_mutex_t* mutex) {
    return pthread_cond_wait(&cond->data, &mutex->data);
}

int platform_cond_broadcast(platform_cond_t* cond) {
    return pthread_cond_broadcast(&cond->data);
}

int platform_thread_create(platform_thread_t* thread,
                           void *(*run)(void *), void* data)
{
    return pthread_create(&thread->data, NULL, run, data);
}

int platform_thread_join(platform_thread_t* thread) {
    return pthread_join(thread->data, NULL);
}
