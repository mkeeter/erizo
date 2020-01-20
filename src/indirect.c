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
out vec3 vert_pos;

void main() {
    vec4 vpos = view * model * vec4(pos, 1.0f);
    vert_pos = vpos.xyz;

    gl_Position = proj * vpos;
    ec_pos = gl_Position.xyz;
}
);

static const GLchar* INDIRECT_FS_SRC = GLSL(330,
in vec3 ec_pos;
in vec3 vert_pos;

uniform mat4 proj;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;

void main() {
    vec3 n_screen = cross(dFdx(ec_pos), dFdy(ec_pos));
    n_screen.z *= proj[2][2];

    out_pos = vert_pos;
    out_pos.z *= -1.0f;
    out_pos = out_pos / 2.0f + 0.5f;

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

    // Position and normal buffers
    for (unsigned i=0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, indirect->tex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0,
                     GL_RGB, GL_FLOAT, NULL);
        log_gl_error();
    }

    glBindRenderbuffer(GL_RENDERBUFFER, indirect->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}

indirect_t* indirect_new(uint32_t width, uint32_t height) {
    OBJECT_ALLOC(indirect);
    indirect->shader = shader_new(INDIRECT_VS_SRC, NULL, INDIRECT_FS_SRC);
    indirect->u_camera = camera_get_uniforms(indirect->shader.prog);

    glGenFramebuffers(1, &indirect->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, indirect->fbo);

    glGenTextures(2, indirect->tex);
    glGenRenderbuffers(1, &indirect->rbo);

    indirect_resize(indirect, width, height);

    // Bind the color output buffers, both of which are textures
    for (unsigned i=0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, indirect->tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, indirect->tex[i], 0);
    }

    // Bind the depth buffer, which is write-only (so we use a renderbuffer)
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, indirect->rbo);

    // Configure the framebuffer to draw to two color buffers
    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

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

void indirect_draw(indirect_t* indirect, model_t* model, camera_t* camera) {
    glEnable(GL_DEPTH_TEST);
    glUseProgram(indirect->shader.prog);
    glBindVertexArray(model->vao);

    camera_bind(camera, indirect->u_camera);

    glBindFramebuffer(GL_FRAMEBUFFER, indirect->fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);
}

void indirect_blit(indirect_t* indirect)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, indirect->fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, 640, 640, 0, 0, 640, 640,  GL_COLOR_BUFFER_BIT, GL_LINEAR);
    log_gl_error();
}
