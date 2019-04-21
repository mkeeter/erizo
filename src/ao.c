#include "ao.h"
#include "camera.h"
#include "model.h"
#include "log.h"
#include "object.h"
#include "shader.h"

static const GLchar* AO_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main() {
    mat4 m = proj * view * model;
    gl_Position = m * vec4(pos, 1.0f);
}
);

static const GLchar* AO_FS_SRC = GLSL(330,
out vec4 out_color;

void main() {
    out_color = vec4(1.0f, 0.0f, 0.0f, 0.0f);
});

ao_t* ao_new(unsigned render_size, unsigned vol_logsize) {
    OBJECT_ALLOC(ao);
    ao->render_size = render_size;
    ao->vol_logsize = vol_logsize;
    if (vol_logsize & 1) {
        log_error_and_abort("vol_logsize (%u) must be divisible by 2",
                             vol_logsize);
    }

    //  Generate and bind a framebuffer
    glGenFramebuffers(1, &ao->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ao->fbo);

    //  Generate and bind a depth buffer
    glGenRenderbuffers(1, &ao->depth);
    glBindRenderbuffer(GL_RENDERBUFFER, ao->depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                          render_size, render_size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, ao->depth);

    // Build the texture for conventional rendering
    glGenTextures(1, &ao->tex);
    glBindTexture(GL_TEXTURE_2D, ao->tex);
    glTexImage2D(GL_TEXTURE_2D,
                 0,                 // level
                 GL_RED,            // internal format
                 render_size,       // width
                 render_size,       // height
                 0,                 // border
                 GL_RED,            // format
                 GL_UNSIGNED_INT,   // type
                 NULL);             // data
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Generate and configure the volumetric textures
    glGenTextures(2, ao->vol);
    const unsigned vol_size = 1 << (vol_logsize / 2 * 3);
    for (unsigned i=0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, ao->vol[i]);
        glTexImage2D(GL_TEXTURE_2D,
                     0,                 // level
                     GL_RGBA,           // internal format
                     vol_size,          // width
                     vol_size,          // height
                     0,                 // border
                     GL_RGBA,           // format
                     GL_FLOAT,          // type
                     NULL);             // data
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    ao->vs = shader_build(AO_VS_SRC, GL_VERTEX_SHADER);
    ao->fs = shader_build(AO_FS_SRC, GL_FRAGMENT_SHADER);
    ao->prog = shader_link_vf(ao->vs, ao->fs);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return ao;
}

void ao_render(ao_t* ao, model_t* model, camera_t* camera) {
    glBindFramebuffer(GL_FRAMEBUFFER, ao->fbo);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(ao->prog);
    glBindVertexArray(model->vao);

    CAMERA_UNIFORM_MAT(ao, model);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ao->tex, 0);
    GLenum draw_buf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &draw_buf);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char* err = NULL;
        switch (status) {
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_UNDEFINED);
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_UNSUPPORTED);
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
            LOG_GL_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
            default: break;
        }
        if (err) {
            log_error("Framebuffer error: %s (0x%x)", err, status);
        } else {
            log_error("Unknown framebuffer error: 0x%x", status);
        }
    }

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ao_delete(ao_t* ao) {
    glDeleteFramebuffers(1, &ao->fbo);
    glDeleteRenderbuffers(1, &ao->depth);
    glDeleteTextures(1, &ao->tex);
    glDeleteTextures(2,  ao->vol);
}
