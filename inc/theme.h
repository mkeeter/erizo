typedef struct theme_ {
    float upper_left[3];
    float upper_right[3];
    float lower_left[3];
    float lower_right[3];

    float key[3];
    float fill[3];
    float base[3];
} theme_t;

theme_t* theme_new_solarized();
theme_t* theme_new_nord();
theme_t* theme_new_gruvbox();

#define THEME_UNIFORM_COLOR(m, u) glUniform3fv(m->u_##u, 1, theme->u)
