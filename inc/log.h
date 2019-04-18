#include "base.h"

typedef enum {
    LOG_TRACE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} log_type_t;

void log_lock();
void log_unlock();
FILE* log_preamble(log_type_t t, const char* file, int line);

#define log_print(t, ...) do {                          \
    log_lock();                                         \
    FILE* out = log_preamble(t, __FILE__, __LINE__);    \
    fprintf(out, __VA_ARGS__);                          \
    fprintf(out, "\n");                                 \
    log_unlock();                                       \
} while (0)

#define log_trace(...)  log_print(LOG_TRACE, __VA_ARGS__)
#define log_info(...)   log_print(LOG_INFO,  __VA_ARGS__)
#define log_warn(...)   log_print(LOG_WARN,  __VA_ARGS__)
#define log_error(...)  log_print(LOG_ERROR, __VA_ARGS__)
#define log_error_and_abort(...) do {                           \
    log_print(LOG_ERROR, __VA_ARGS__);                          \
    exit(-1);                                                   \
} while (0)
