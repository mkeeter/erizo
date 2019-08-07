#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "app.h"
#include "log.h"
#include "object.h"
#include "platform.h"

const char* platform_mmap(const char* filename, size_t* size) {
    (void)filename;
    (void)size;
    return NULL;
}

void platform_munmap(const char* data, size_t size) {
    (void)data;
    (void)size;
}

int64_t platform_get_time(void) {
    LARGE_INTEGER i;
    if (!QueryPerformanceCounter(&i)) {
        log_error_and_abort("Failed to get time");
    }
    return i.QuadPart;
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

////////////////////////////////////////////////////////////////////////////////

struct platform_thread_ {
    HANDLE data;
};

/* The pthreads API (and thus the platform_thread API) expects a function
 * returning void*, while the win32 API returns DWORD.  We use this small
 * struct to translate from one to the other. */
typedef struct {
    void *(*run)(void *);
    void* data;
} thread_data_t;

DWORD WINAPI thread_runner(LPVOID data_) {
    thread_data_t* data = data_;
    (data->run)(data->data);
    free(data_);
    return 0;
}

platform_thread_t*  platform_thread_new(void *(*run)(void *),
                                        void* data)
{
    OBJECT_ALLOC(platform_thread);

    thread_data_t* d = calloc(1, sizeof(thread_data_t));
    d->run = run;
    d->data = data;

    platform_thread->data = CreateThread(NULL, 0, &thread_runner, d, 0, NULL);
    if (platform_thread->data == NULL) {
        free(platform_thread);
        return NULL;
    }
    return platform_thread;
}

void platform_thread_delete(platform_thread_t* thread) {
    CloseHandle(thread->data);
    free(thread);
}

int platform_thread_join(platform_thread_t* thread) {
    return WaitForSingleObject(thread->data, INFINITE);
}

////////////////////////////////////////////////////////////////////////////////

void platform_init(app_t* app, int argc, char** argv) {
    (void)app;
    (void)argc;
    (void)argv;
}

void platform_window_bind(GLFWwindow* window) {
    (void)window;
}

/*  Shows a warning dialog with the given text */
void platform_warning(const char* title, const char* text) {
    log_error(text);
}

/*  Returns the filename portion of a full path */
const char* platform_filename(const char* filepath) {
    return filepath;
}
