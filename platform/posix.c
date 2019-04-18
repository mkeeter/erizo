#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "log.h"
#include "object.h"
#include "platform.h"

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

typedef struct platform_thread_ {
    pthread_t data;
} platform_thread_t;

typedef struct platform_mutex_ {
    pthread_mutex_t data;
} platform_mutex_t;

typedef struct platform_cond_ {
    pthread_cond_t data;
} platform_cond_t;

platform_mutex_t* platform_mutex_new() {
    OBJECT_ALLOC(platform_mutex);
    if (pthread_mutex_init(&platform_mutex->data, NULL)) {
        log_error_and_abort("Failed to initialize mutex");
    }
    return platform_mutex;
}

void platform_mutex_delete(platform_mutex_t* mutex) {
    if (pthread_mutex_destroy(&mutex->data)) {
        log_error_and_abort("Failed to destroy mutex");
    }
    free(mutex);
}

platform_cond_t* platform_cond_new() {
    OBJECT_ALLOC(platform_cond);
    if (pthread_cond_init(&platform_cond->data, NULL)) {
        log_error_and_abort("Failed to initialize condition variable");
    }
    return platform_cond;
}

void platform_cond_delete(platform_cond_t* cond) {
    if (pthread_cond_destroy(&cond->data)) {
        log_error_and_abort("Failed to destroy condition variable");
    }
    free(cond);
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

platform_thread_t* platform_thread_new(void *(*run)(void *), void* data)
{
    OBJECT_ALLOC(platform_thread);
    if (pthread_create(&platform_thread->data, NULL, run, data)) {
        log_error_and_abort("Failed to create thread");
    }
    return platform_thread;
}

int platform_thread_join(platform_thread_t* thread) {
    return pthread_join(thread->data, NULL);
}

void platform_thread_delete(platform_thread_t* thread) {
    free(thread);
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
