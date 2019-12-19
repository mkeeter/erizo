#include "camera.h"
#include "draw.h"
#include "log.h"
#include "theme.h"
#include "object.h"
#include "model.h"
#include "shader.h"

/*  draw_t represents a bunch of data necessary to draw to the viewport
 *  using a standard camera and lighting setup. */
struct draw_ {
    /*  Shader program */
    shader_t shader;

    /*  Uniform locations for matrices */
    camera_uniforms_t u_camera;

    /*  Uniform locations for lighting */
    theme_uniforms_t u_theme;
};

draw_t* draw_new(const char* vs, const char* gs, const char* fs) {
    OBJECT_ALLOC(draw);
    draw->shader = shader_new(vs, gs, fs);

    draw->u_camera = camera_get_uniforms(draw->shader.prog);
    draw->u_theme = theme_get_uniforms(draw->shader.prog);

    log_gl_error();
    return draw;
}

void draw_delete(draw_t* draw) {
    shader_deinit(draw->shader);
    free(draw);
}

void draw(draw_t* draw, model_t* model, camera_t* camera, theme_t* theme)
{
    glEnable(GL_DEPTH_TEST);
    glUseProgram(draw->shader.prog);
    glBindVertexArray(model->vao);

    camera_bind(camera, draw->u_camera);
    theme_bind(theme, draw->u_theme);

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();
}
