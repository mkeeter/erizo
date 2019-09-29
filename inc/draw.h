#include "base.h"

/* Forward declarations */
struct camera_;
struct model_;
struct theme_;

/*  draw_t represents a bunch of data necessary to draw to the viewport
 *  using a standard camera and lighting setup. */
typedef struct draw_ {
    /*  Shader program */
    GLuint vs;
    GLuint gs;
    GLuint fs;
    GLuint prog;

    /*  Uniform locations for matrices */
    GLint u_proj;
    GLint u_view;
    GLint u_model;

    /*  Uniform locations for lighting */
    GLint u_key;
    GLint u_fill;
    GLint u_base;
} draw_t;

/*  Constructs a new draw object.  gs is optional and may be NULL */
draw_t* draw_new(const char* vs, const char* gs, const char* fs);

void draw_delete(draw_t* d);

/*  After vs, gs, and fs are populated, links the shader and
 *  saves the uniform locations. */
void draw_link(draw_t* d);

void draw(draw_t* draw, struct model_* model,
          struct camera_* camera, struct theme_* theme);
