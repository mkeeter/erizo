#include "camera.h"
#include "draw.h"
#include "log.h"
#include "theme.h"
#include "object.h"
#include "model.h"
#include "shader.h"

draw_t* draw_new(const char* vs, const char* gs, const char* fs) {
    OBJECT_ALLOC(draw);
    draw->vs = shader_build(vs, GL_VERTEX_SHADER);
    draw->fs = shader_build(fs, GL_FRAGMENT_SHADER);
    if (gs) {
        draw->gs = shader_build(gs, GL_GEOMETRY_SHADER);
        draw->prog = shader_link_vgf(draw->vs, draw->gs, draw->fs);
    } else {
        draw->prog = shader_link_vf(draw->vs, draw->fs);
    }

    SHADER_GET_UNIFORM_LOC(draw, proj);
    SHADER_GET_UNIFORM_LOC(draw, view);
    SHADER_GET_UNIFORM_LOC(draw, model);

    SHADER_GET_UNIFORM_LOC(draw, key);
    SHADER_GET_UNIFORM_LOC(draw, fill);
    SHADER_GET_UNIFORM_LOC(draw, base);

    log_gl_error();
    return draw;
}

void draw_delete(draw_t* draw) {
    glDeleteShader(draw->vs);
    glDeleteShader(draw->gs);
    glDeleteShader(draw->fs);
    glDeleteProgram(draw->prog);
    free(draw);
}

void draw(draw_t* draw, model_t* model, camera_t* camera, theme_t* theme)
{
    glEnable(GL_DEPTH_TEST);
    glUseProgram(draw->prog);
    glBindVertexArray(model->vao);

    camera_bind(camera, draw);
    theme_bind(theme, draw);

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();
}
