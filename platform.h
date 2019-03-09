#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

const char* platform_mmap(const char* filename);
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
    TERM_COLOR_WHITE,
} platform_terminal_color_t;
void platform_set_terminal_color(platform_terminal_color_t c);
void platform_clear_terminal_color();

/*  Threading API is a thin wrapper around pthreads */
struct platform_thread;
struct platform_mutex;
struct platform_cond;

int platform_mutex_init(struct platform_mutex* mutex);
int platform_mutex_lock(struct platform_mutex* mutex);
int platform_mutex_unlock(struct platform_mutex* mutex);

int platform_cond_init(struct platform_cond* cond);
int platform_cond_wait(struct platform_cond* cond,
                       struct platform_mutex* mutex);
int platform_cond_broadcast(struct platform_cond* cond);

int platform_thread_create(struct platform_thread* thread,
                           void *(*run)(void *), void* data);
int platform_thread_join(struct platform_thread* thread);

#endif
