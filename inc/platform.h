#include "base.h"

struct app_;

const char* platform_mmap(const char* filename, size_t* size);
void platform_munmap(const char* data, size_t size);

void platform_get_time(int64_t* sec, int32_t* usec);

/*  Based on 8-color ANSI terminals */
typedef enum {
    TERM_COLOR_BLACK,
    TERM_COLOR_RED,
    TERM_COLOR_GREEN,
    TERM_COLOR_YELLOW,
    TERM_COLOR_BLUE,
    TERM_COLOR_MAGENTA,
    TERM_COLOR_CYAN,
    TERM_COLOR_WHITE
} platform_terminal_color_t;
void platform_set_terminal_color(FILE* f, platform_terminal_color_t c);
void platform_clear_terminal_color(FILE* f);

////////////////////////////////////////////////////////////////////////////////

/*  Threading API is a thin wrapper around pthreads */
struct platform_mutex_;
struct platform_cond_;
struct platform_thread_;

struct platform_mutex_* platform_mutex_new();
void platform_mutex_delete(struct platform_mutex_* mutex);
int platform_mutex_lock(struct platform_mutex_* mutex);
int platform_mutex_unlock(struct platform_mutex_* mutex);

struct platform_cond_* platform_cond_new();
void platform_cond_delete(struct platform_cond_* cond);
int platform_cond_wait(struct platform_cond_* cond,
                       struct platform_mutex_* mutex);
int platform_cond_broadcast(struct platform_cond_* cond);

struct platform_thread_*  platform_thread_new(void *(*run)(void *),
                                              void* data);
void platform_thread_delete(struct platform_thread_* mutex);
int platform_thread_join(struct platform_thread_* thread);

////////////////////////////////////////////////////////////////////////////////

/*  Initializes the menu and other native features */
void platform_init(struct app_* app, int argc, char** argv);
void platform_window_bind(GLFWwindow* window);

/*  Shows a warning dialog with the given text */
void platform_warning(const char* title, const char* text);

/*  Returns the filename portion of a full path */
const char* platform_filename(const char* filepath);
