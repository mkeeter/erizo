/*  Forward declarations */
struct draw_;

typedef struct theme_ {
    float corners[4][3];

    float key[3];
    float fill[3];
    float base[3];
} theme_t;

theme_t* theme_new_solarized();
theme_t* theme_new_nord();
theme_t* theme_new_gruvbox();

void theme_bind(theme_t* theme, struct draw_* draw);
