#include "ao.h"
#include "bitmap.h"
#include "camera.h"
#include "model.h"
#include "log.h"
#include "object.h"
#include "shader.h"

static const GLchar* AO_DEPTH_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

uniform mat4 model;

void main() {
    gl_Position = model * vec4(pos, 1.0f);
}
);

static const GLchar* AO_DEPTH_FS_SRC = GLSL(330,
out vec4 out_color;

void main() {
    out_color = vec4(1.0f - gl_FragCoord.z, 0.0f, 0.0f, 0.0f);
});

static void ao_save_depth_bitmap(ao_t* ao, const char* filename) {
    FILE* out = fopen(filename, "w");
    if (!out) {
        log_error_and_abort("Could not open '%s'", filename);
    }

    // Read back the texture from the GPU
    glBindTexture(GL_TEXTURE_2D, ao->depth_tex);
    GLuint* pixels = malloc(ao->depth_size * ao->depth_size * sizeof(GLuint));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_INT, pixels);

    // Write the image
    bitmap_write_header(out, ao->depth_size, ao->depth_size);
    bitmap_write_depth(out, ao->depth_size, ao->depth_size, pixels);

    free(pixels);
    fclose(out);
    return;
}

ao_t* ao_new(unsigned depth_size, unsigned vol_logsize) {
    OBJECT_ALLOC(ao);
    ao->depth_size = depth_size;
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
                          depth_size, depth_size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, ao->depth);

    // Build the texture for conventional rendering
    glGenTextures(1, &ao->depth_tex);
    glBindTexture(GL_TEXTURE_2D, ao->depth_tex);
    glTexImage2D(GL_TEXTURE_2D,
                 0,                 // level
                 GL_RED,            // internal format
                 depth_size,        // width
                 depth_size,        // height
                 0,                 // border
                 GL_RED,            // format
                 GL_UNSIGNED_INT,   // type
                 NULL);             // data
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Generate and configure the volumetric textures
    glGenTextures(2, ao->vol_tex);
    const unsigned vol_size = 1 << (vol_logsize / 2 * 3);
    for (unsigned i=0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, ao->vol_tex[i]);
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

    {   /*  We build a set of square tiles in X and Y, each representing a
            single Z slice of the model, then upload this data to the GPU. */
        float (*squares) = malloc(sizeof(float)
                * 2 // triangles per square
                * 3 // vertices per triangle
                * 5 // data per vertex (x, y, vx, vy, vz)
                * (1 << vol_logsize));
        float* ptr = squares;
        const unsigned tiles_per_edge = 1 << (vol_logsize / 2);
        for (unsigned i=0; i < tiles_per_edge; ++i) {
            const float xmin =  i      / (float)tiles_per_edge;
            const float xmax = (i + 1) / (float)tiles_per_edge;
            for (unsigned j=0; j < tiles_per_edge; ++j) {
                const float ymin =  j      / (float)tiles_per_edge;
                const float ymax = (j + 1) / (float)tiles_per_edge;
                const float z = ((i * tiles_per_edge) + j) /
                                (float)((1 << vol_logsize) - 1);

                /* Vertices are as follows:
                 *  2----3
                 *  |    |
                 *  0----1  */
                const float vs[4][5] = {
                //    x      y       vx      vy    vz
                    {xmin,  ymin,   -1.0,   -1.0,   z},
                    {xmax,  ymin,    1.0,   -1.0,   z},
                    {xmin,  ymax,   -1.0,    1.0,   z},
                    {xmax,  ymax,    1.0,    1.0,   z}};
                const unsigned triangles[2][3] = {{0, 1, 2}, {2, 1, 3}};
                for (unsigned t=0; t < 2; ++t) {
                    for (unsigned v=0; v < 3; ++v) {
                        memcpy(ptr, vs[triangles[t][v]], 5 * sizeof(float));
                        ptr += 5;
                    }
                }
            }
        }
    }

    // Build and link the AO shader programs
    ao->depth_vs = shader_build(AO_DEPTH_VS_SRC, GL_VERTEX_SHADER);
    ao->depth_fs = shader_build(AO_DEPTH_FS_SRC, GL_FRAGMENT_SHADER);
    ao->depth_prog = shader_link_vf(ao->depth_vs, ao->depth_fs);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return ao;
}

void ao_render(ao_t* ao, model_t* model, camera_t* camera) {
    glBindFramebuffer(GL_FRAMEBUFFER, ao->fbo);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(ao->depth_prog);
    glBindVertexArray(model->vao);

    CAMERA_UNIFORM_MAT(ao, model);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         ao->depth_tex, 0);
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

    GLint prev_viewport_size[4];
    glGetIntegerv(GL_VIEWPORT, prev_viewport_size);
    log_gl_error();
    glViewport(0, 0, ao->depth_size, ao->depth_size);
    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);

    // Restore previous render settings
    glViewport(0, 0, prev_viewport_size[2], prev_viewport_size[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    log_gl_error();

    ao_save_depth_bitmap(ao, "depth.bmp");
}

void ao_delete(ao_t* ao) {
    glDeleteFramebuffers(1, &ao->fbo);
    glDeleteRenderbuffers(1, &ao->depth);
    glDeleteTextures(1, &ao->depth_tex);
    glDeleteTextures(2,  ao->vol_tex);
    glDeleteShader(ao->depth_vs);
    glDeleteShader(ao->depth_fs);
    glDeleteShader(ao->depth_prog);
    free(ao);
}
