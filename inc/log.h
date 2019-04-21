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

#define log_gl_error() do {                                         \
    GLenum err = glGetError();                                      \
    if (err != GL_NO_ERROR) {                                       \
        const char* str = NULL;                                     \
        switch (err) {                                              \
            case GL_NO_ERROR:                                       \
                str = "GL_NO_ERROR"; break;                         \
            case GL_INVALID_ENUM:                                   \
                str = "GL_INVALID_ENUM"; break;                     \
            case GL_INVALID_VALUE:                                  \
                str = "GL_INVALID_VALUE"; break;                    \
            case GL_INVALID_OPERATION:                              \
                str = "GL_INVALID_OPERATION"; break;                \
            case GL_INVALID_FRAMEBUFFER_OPERATION:                  \
                str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;    \
            case GL_OUT_OF_MEMORY:                                  \
                str = "GL_OUT_OF_MEMORY"; break;                    \
            default: break;                                         \
        }                                                           \
        if (str) {                                                  \
            log_error("OpenGL error: %s (0x%x)", str, err);         \
        } else {                                                    \
            log_error("Unknown OpenGL error: %i", err);             \
        }                                                           \
    }                                                               \
} while(0)
