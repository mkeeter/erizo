typedef enum {
    LOG_TRACE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} log_type_t;

void log_print(log_type_t t, const char* file, int line, const char *fmt, ...);

#define log_trace(...)  log_print(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__);
#define log_info(...)   log_print(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__);
#define log_warn(...)   log_print(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__);
#define log_error(...)  log_print(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);
#define log_error_and_abort(...) do {                           \
    log_print(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);      \
    exit(-1);                                                   \
} while (0);
