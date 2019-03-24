#ifndef PLATFORM_H
#define PLATFORM_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_DARWIN)
#include "platform_unix.h"
#else
#error "Not yet ported to this platform!"
#endif

struct app_;
struct instance_;

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

/*  Threading API is a thin wrapper around pthreads */
int platform_mutex_init(platform_mutex_t* mutex);
int platform_mutex_destroy(platform_mutex_t* mutex);
int platform_mutex_lock(platform_mutex_t* mutex);
int platform_mutex_unlock(platform_mutex_t* mutex);

int platform_cond_init(platform_cond_t* cond);
int platform_cond_destroy(platform_cond_t* cond);
int platform_cond_wait(platform_cond_t* cond,
                       platform_mutex_t* mutex);
int platform_cond_broadcast(platform_cond_t* cond);

int platform_thread_create(platform_thread_t* thread,
                           void *(*run)(void *), void* data);
int platform_thread_join(platform_thread_t* thread);

/*  Initializes the menu and other native features */
void platform_init(struct app_* app, int argc, char** argv);
void platform_window_bind(GLFWwindow* window);

/*  Shows a warning dialog with the given text */
void platform_warning(const char* title, const char* text);

/*  Returns the filename portion of a full path */
const char* platform_filename(const char* filepath);

#endif
