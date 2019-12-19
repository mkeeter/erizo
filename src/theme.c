#include "platform.h"

#include "log.h"
#include "draw.h"
#include "object.h"
#include "theme.h"
#include "shader.h"

static void from_hex(const char* hex, float f[3]) {
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
    for (unsigned i=0; i < 3; ++i) {
        f[i] = c[i] / 255.0f;
    }
}

theme_t* theme_new_solarized() {
    OBJECT_ALLOC(theme);

    /*  Backdrop colors */
    from_hex("002833", theme->corners[0]);
    from_hex("002833", theme->corners[1]);
    from_hex("003440", theme->corners[2]);
    from_hex("002833", theme->corners[3]);

    /*  Model colors */
    from_hex("fdf6e3", theme->key);
    from_hex("eee8d5", theme->fill);
    from_hex("657b83", theme->base);

    return theme;
}

theme_t* theme_new_nord() {
    OBJECT_ALLOC(theme);

    /*  Backdrop colors */
    from_hex("2e3440", theme->corners[0]);
    from_hex("2e3440", theme->corners[1]);
    from_hex("3b4252", theme->corners[2]);
    from_hex("2e3440", theme->corners[3]);

    /*  Model colors */
    from_hex("eceff4", theme->key);
    from_hex("d8dee9", theme->fill);
    from_hex("4c566a", theme->base);

    return theme;
}

theme_t* theme_new_gruvbox() {
    OBJECT_ALLOC(theme);

    /*  Backdrop colors */
    from_hex("1d2021", theme->corners[0]);
    from_hex("1d2021", theme->corners[1]);
    from_hex("3c3836", theme->corners[2]);
    from_hex("1d2021", theme->corners[3]);

    /*  Model colors */
    from_hex("fbf1c7", theme->key);
    from_hex("bdae93", theme->fill);
    from_hex("665c54", theme->base);

    return theme;
}

theme_uniforms_t theme_get_uniforms(GLuint prog) {
    theme_uniforms_t u;
    SHADER_GET_UNIFORM(key);
    SHADER_GET_UNIFORM(fill);
    SHADER_GET_UNIFORM(base);
    return u;
}

void theme_bind(theme_t* theme, theme_uniforms_t u) {
    glUniform3fv(u.key,  1, theme->key);
    glUniform3fv(u.fill, 1, theme->fill);
    glUniform3fv(u.base, 1, theme->base);
}
