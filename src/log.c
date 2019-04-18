#include "platform.h"
#include "log.h"
#include "log_align.h"

platform_terminal_color_t log_message_color(log_type_t t) {
    switch (t) {
        case LOG_TRACE: return TERM_COLOR_BLUE;
        case LOG_INFO: return TERM_COLOR_GREEN;
        case LOG_WARN: return TERM_COLOR_YELLOW;
        case LOG_ERROR: return TERM_COLOR_RED;
    }
}

void log_print(log_type_t t, const char* file, int line, const char *fmt, ...)
{
    static int64_t start_sec = -1;
    static int32_t start_usec = -1;
    static struct platform_mutex_* mut;

    if (start_sec == -1) {
        platform_get_time(&start_sec, &start_usec);
        mut = platform_mutex_new();
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

    const char* filename = platform_filename(file);

    /*  Figure out how much to pad the filename + line number */
    int pad = 0;
    for (int i=line; i; i /= 10, pad++);
    pad += strlen(filename);
    pad = LOG_ALIGN - pad;

    platform_mutex_lock(mut);
        platform_set_terminal_color(out, log_message_color(t));
        fprintf(out, "[erizo]");

        platform_set_terminal_color(out, TERM_COLOR_WHITE);
        fprintf(out, " (%li.%06i) ", dt_sec, dt_usec);

        platform_clear_terminal_color(out);
        fprintf(out, "%s:%i ", filename, line);

        while (pad--) {
            fputc(' ', out);
        }

        fprintf(out, "| ");

        va_list args;
        va_start(args, fmt);
        vfprintf(out, fmt, args);
        va_end(args);

        fprintf(out, "\n");
    platform_mutex_unlock(mut);
}
