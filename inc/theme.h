typedef struct theme_ {
    const char* upper_left;
    const char* upper_right;
    const char* lower_left;
    const char* lower_right;

    const char* key;
    const char* fill;
    const char* base;
} theme_t;

theme_t* theme_new_solarized();
theme_t* theme_new_nord();
