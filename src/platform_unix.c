#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "platform.h"
#include "log.h"

const char* platform_mmap(const char* filename, size_t* size) {
    int stl_fd = open(filename, O_RDONLY);
    if (stl_fd == -1) {
        log_error("open failed (errno: %i)", errno);
        return NULL;
    }
    struct stat s;
    if (fstat(stl_fd, &s)) {
        log_error("fstat failed (errno: %i)", errno);
        close(stl_fd);
        return NULL;
    }
    *size = s.st_size;

    const void* ptr = mmap(0, *size, PROT_READ, MAP_PRIVATE, stl_fd, 0);
    close(stl_fd);
    if (ptr == (void*)-1) {
        log_error("mmap failed (errno: %i)", errno);
        return NULL;
    }
    return (const char*)ptr;
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

void platform_set_terminal_color(FILE* f, platform_terminal_color_t c) {
    fprintf(f, "\x1b[3%im", c - TERM_COLOR_BLACK);
}

void platform_clear_terminal_color(FILE* f) {
    fprintf(f, "\x1b[0m");
}

int platform_mutex_init(platform_mutex_t* mutex) {
    return pthread_mutex_init(&mutex->data, NULL);
}

int platform_mutex_destroy(platform_mutex_t* mutex) {
    return pthread_mutex_destroy(&mutex->data);
}

int platform_cond_init(platform_cond_t* cond) {
    return pthread_cond_init(&cond->data, NULL);
}

int platform_cond_destroy(platform_cond_t* cond) {
    return pthread_cond_destroy(&cond->data);
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

const char* platform_filename(const char* filepath) {
    const char* target = filepath;
    while (1) {
        const char* next = strchr(target, '/');
        if (next) {
            target = next + 1;
        } else {
            break;
        }
    }
    return target;
}
