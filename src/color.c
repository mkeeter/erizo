#include "color.h"
#include "log.h"

void color_uniform_hex(GLuint u, const char* hex) {
    if (strlen(hex) != 6) {
        log_error_and_abort("Invalid hex string '%s'", hex);
    }
    uint8_t c[3] = {0};
    for (unsigned i=0; i < 6; ++i) {
        unsigned v = 0;
        if (hex[i] >= 'a' && hex[i] <= 'f') {
            v = hex[i] - 'a' + 10;
        } else if (hex[i] >= 'A' && hex[i] <= 'F') {
            v = hex[i] - 'A' + 10;
        } else if (hex[i] >= '0' && hex[i] <= '9') {
            v = hex[i] - '0';
        } else {
            log_error_and_abort("Invalid hex character '%c'", hex[i]);
        }
        c[i / 2] = (c[i / 2] << 4) + v;
    }
    float f[3];
    for (unsigned i=0; i < 3; ++i) {
        f[i] = c[i] / 255.0f;
    }
    glUniform3fv(u,  1, f);
}
