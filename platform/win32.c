#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform.h"
#include "object.h"
#include "log.h"

const char* platform_mmap(const char* filename, size_t* size) {
    (void)filename;
    (void)size;
    return NULL;
}

void platform_munmap(const char* data, size_t size) {
    (void)data;
    (void)size;
}

void platform_set_terminal_color(FILE* f, platform_terminal_color_t c) {
    (void)f;
    (void)c;
}

void platform_clear_terminal_color(FILE* f) {
    (void)f;
}

struct platform_mutex_ {
    CRITICAL_SECTION data;
};

platform_mutex_t* platform_mutex_new() {
    OBJECT_ALLOC(platform_mutex);
    InitializeCriticalSection(&platform_mutex->data);
    return platform_mutex;
}

void platform_mutex_delete(platform_mutex_t* mutex) {
    free(mutex);
}

int platform_mutex_lock(platform_mutex_t* mutex) {
    EnterCriticalSection(&mutex->data);
    return 0;
}

int platform_mutex_unlock(platform_mutex_t* mutex) {
    LeaveCriticalSection(&mutex->data);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

struct platform_cond_ {
    CONDITION_VARIABLE data;
};

platform_cond_t* platform_cond_new() {
    OBJECT_ALLOC(platform_cond);
    InitializeConditionVariable(&platform_cond->data);
    return platform_cond;
}

void platform_cond_delete(platform_cond_t* cond) {
    free(cond);
}

int platform_cond_wait(platform_cond_t* cond,
                       platform_mutex_t* mutex)
{
    return !SleepConditionVariableCS(&cond->data, &mutex->data, INFINITE);
}

int platform_cond_broadcast(platform_cond_t* cond) {
    WakeAllConditionVariable(&cond->data);
    return 0;
}
