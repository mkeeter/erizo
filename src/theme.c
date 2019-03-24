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

theme_t* theme_new_nord() {
    OBJECT_ALLOC(theme);

    /*  Backdrop colors */
    theme->upper_left  = "3b4252";
    theme->upper_right = "2e3440";
    theme->lower_left  = "2e3440";
    theme->lower_right = "2e3440";

    /*  Model colors */
    theme->key  = "eceff4";
    theme->fill = "d8dee9";
    theme->base = "4c566a";

    return theme;
}

theme_t* theme_new_gruvbox() {
    OBJECT_ALLOC(theme);

    /*  Backdrop colors */
    theme->upper_left  = "3c3836";
    theme->upper_right = "1d2021";
    theme->lower_left  = "1d2021";
    theme->lower_right = "1d2021";

    /*  Model colors */
    theme->key  = "fbf1c7";
    theme->fill = "bdae93";
    theme->base = "665c54";

    return theme;
}
