#include "platform.h"

typedef enum {
    LOG_TRACE,
    LOG_INFO,
    LOG_ERROR,
} log_type_t;

platform_terminal_color_t log_message_color(log_type_t t) {
    switch (t) {
        case LOG_TRACE: return TERM_COLOR_BLUE;
        case LOG_INFO: return TERM_COLOR_GREEN;
        case LOG_ERROR: return TERM_COLOR_RED;
    }
}

void log_print(log_type_t t, const char *fmt, va_list args)
{
    static int64_t start_sec = -1;
    static int32_t start_usec = -1;
    static platform_mutex_t mut;

    if (start_sec == -1) {
        platform_get_time(&start_sec, &start_usec);
        platform_mutex_init(&mut);
    }

    int64_t now_sec;
    int32_t now_usec;
    platform_get_time(&now_sec, &now_usec);

    long int dt_sec = now_sec - start_sec;
    int dt_usec = now_usec - start_usec;
    if (dt_usec < 0) {
        dt_usec += 1000000;
        dt_sec -= 1;
    }

    FILE* out = (t == LOG_ERROR) ? stderr : stdout;
    platform_mutex_lock(&mut);
        platform_set_terminal_color(out, log_message_color(t));
        fprintf(out, "[hedgehog]");

        platform_set_terminal_color(out, TERM_COLOR_WHITE);
        fprintf(out, " (%li.%06i) ", dt_sec, dt_usec);

        platform_clear_terminal_color(out);
        vfprintf(out, fmt, args);
        fprintf(out, "\n");
    platform_mutex_unlock(&mut);
}

void log_trace(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_print(LOG_TRACE, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_print(LOG_INFO, fmt, args);
    va_end(args);
}

void log_error_and_abort(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_print(LOG_ERROR, fmt, args);
    va_end(args);
    exit(-1);
}

