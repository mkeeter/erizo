#include "platform.h"

#include "object.h"
#include "theme.h"

theme_t* theme_new_solarized() {
    OBJECT_ALLOC(theme);

    /*  Backdrop colors */
    theme->upper_left  = "003440";
    theme->upper_right = "002833";
    theme->lower_left  = "002833";
    theme->lower_right = "002833";

    /*  Model colors */
    theme->key  = "fdf6e3";
    theme->fill = "eee8d5";
    theme->base = "657b83";

    return theme;
}
