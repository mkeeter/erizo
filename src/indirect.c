#include "camera.h"
#include "indirect.h"
#include "log.h"
#include "model.h"
#include "object.h"
#include "shader.h"

static const GLchar* INDIRECT_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec3 ec_pos;

void main() {
    gl_Position = proj * view * model * vec4(pos, 1.0f);
    ec_pos = gl_Position.xyz;
}
);

static const GLchar* INDIRECT_FS_SRC = GLSL(330,
in vec3 ec_pos;

uniform mat4 proj;

out float out_depth;
out vec3 out_normal;

void main() {
    vec3 n_screen = cross(dFdx(ec_pos), dFdy(ec_pos));
    n_screen.z *= proj[2][2];

    out_depth = gl_FragDepth;
    out_normal = normalize(n_screen);
}
);

struct indirect_ {
    camera_uniforms_t u_camera;
    shader_t shader;
    GLuint fbo;
    GLuint tex[2];  /* position, normal */
    GLuint rbo;     /* depth + stencil buffer */
};

void indirect_delete(indirect_t* indirect) {
    glDeleteFramebuffers(1, &indirect->fbo);
    shader_deinit(indirect->shader);
    glDeleteTextures(2, indirect->tex);
    glDeleteRenderbuffers(1, &indirect->rbo);
    free(indirect);
}


void indirect_resize(indirect_t* indirect, uint32_t width, uint32_t height) {
    glBindFramebuffer(GL_FRAMEBUFFER, indirect->fbo);

    // Depth
    glBindTexture(GL_TEXTURE_2D, indirect->tex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0,
                 GL_RED, GL_FLOAT, NULL);
    log_gl_error();

    // Normal
    glBindTexture(GL_TEXTURE_2D, indirect->tex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0,
                 GL_RGB, GL_FLOAT, NULL);
    log_gl_error();

    glBindRenderbuffer(GL_RENDERBUFFER, indirect->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}

indirect_t* indirect_new(uint32_t width, uint32_t height) {
    OBJECT_ALLOC(indirect);
    indirect->shader = shader_new(INDIRECT_VS_SRC, NULL, INDIRECT_FS_SRC);

    glGenFramebuffers(1, &indirect->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, indirect->fbo);

    glGenTextures(2, indirect->tex);
    glGenRenderbuffers(1, &indirect->rbo);

    indirect_resize(indirect, width, height);

    for (unsigned i=0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, indirect->tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, indirect->tex[i], 0);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, indirect->rbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, indirect->rbo);


    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char* err = NULL;
        switch (status) {
#define LOG_FB_ERR_CASE(s) case s: err = #s; break
            LOG_FB_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
            LOG_FB_ERR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
            LOG_FB_ERR_CASE(GL_FRAMEBUFFER_UNSUPPORTED);
            default: break;
#undef LOG_FB_ERR_CASE
        }
        if (err) {
            log_error_and_abort("Framebuffer error: %s (0x%x)", err, status);
        } else {
            log_error_and_abort("Unknown framebuffer error: %u", status);
        }
    }

    return indirect;
}

void indirect_draw(indirect_t* indirect, model_t* model, camera_t* camera)
{
    glEnable(GL_DEPTH_TEST);
    glUseProgram(indirect->shader.prog);
    glBindVertexArray(model->vao);

    camera_bind(camera, indirect->u_camera);

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
    log_gl_error();
}
