#include "base.h"

typedef struct theme_ {
    float corners[4][3];

    float key[3];
    float fill[3];
    float base[3];
} theme_t;

typedef struct theme_uniforms_ {
    GLint key;
    GLint fill;
    GLint base;
} theme_uniforms_t;

theme_t* theme_new_solarized();
theme_t* theme_new_nord();
theme_t* theme_new_gruvbox();

/*  Looks up uniforms for theme binding */
theme_uniforms_t theme_get_uniforms(GLuint prog);

void theme_bind(theme_t* theme, theme_uniforms_t u);
