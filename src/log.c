#include "platform.h"
#include "log.h"
#include "log_align.h"

platform_terminal_color_t log_message_color(log_type_t t) {
    switch (t) {
        case LOG_TRACE: return TERM_COLOR_BLUE;
        case LOG_INFO:  return TERM_COLOR_GREEN;
        case LOG_WARN:  return TERM_COLOR_YELLOW;
        case LOG_ERROR: return TERM_COLOR_RED;
        default:        return TERM_COLOR_WHITE;
    }
}

static struct platform_mutex_* mut = NULL;
static FILE* log_file = NULL;

void log_init() {
    assert(mut == NULL);
    mut = platform_mutex_new();
    if (!platform_is_tty()) {
        log_file = fopen("/tmp/erizo.log", "w");
    }
}

void log_deinit() {
    if (log_file) {
        fclose(log_file);
    }
}

void log_lock() {
    platform_mutex_lock(mut);
}

void log_unlock() {
    platform_mutex_unlock(mut);
}

FILE* log_preamble(log_type_t t, const char* file, int line)
{
    static int64_t start_usec = -1;

    if (start_usec == -1) {
        start_usec = platform_get_time();
    }

    const uint64_t dt_usec = platform_get_time() - start_usec;

    FILE* out;
    if (log_file) {
        out = log_file;
    } else if (t == LOG_ERROR) {
        out = stderr;
    } else {
        out = stdout;
    }

    const char* filename = platform_filename(file);

    /*  Figure out how much to pad the filename + line number */
    int pad = 0;
    for (int i=line; i; i /= 10, pad++);
    pad += strlen(filename);
    pad = LOG_ALIGN - pad - 6;
    assert(pad >= 0);

    if (!log_file) {
        platform_set_terminal_color(out, log_message_color(t));
    }
    fprintf(out, "[erizo]");

    if (!log_file) {
        platform_set_terminal_color(out, TERM_COLOR_WHITE);
    }
    fprintf(out, " (%u.%06u) ", (uint32_t)(dt_usec / 1000000),
                                (uint32_t)(dt_usec % 1000000));

    if (!log_file) {
        platform_clear_terminal_color(out);
    }
    fprintf(out, "%s:%i ", filename, line);

    while (pad--) {
        fputc(' ', out);
    }

    fprintf(out, "| ");
    return out;
}
