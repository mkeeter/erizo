#include "ao.h"
#include "bitmap.h"
#include "camera.h"
#include "icosphere.h"
#include "log.h"
#include "mat.h"
#include "model.h"
#include "object.h"
#include "shader.h"

// Utility function to check framebuffer status
static void check_framebuffer() {
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
}

static void setup_texture() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
}

////////////////////////////////////////////////////////////////////////////////

static const GLchar* AO_DEPTH_VS_SRC = GLSL(330,
layout(location=0) in vec3 pos;

uniform mat4 model;
uniform mat4 view;

void main() {
    gl_Position = view * model * vec4(pos, 1.0f);
}
);

static const GLchar* AO_DEPTH_FS_SRC = GLSL(330,
out vec4 out_color;

void main() {
    out_color = vec4(1.0f - gl_FragCoord.z, 0.0f, 0.0f, 0.0f);
});

static void ao_depth_init(ao_depth_t* d, unsigned size) {
    d->size = size;

    //  Generate and bind a framebuffer
    glGenFramebuffers(1, &d->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, d->fbo);

    //  Generate and bind a depth buffer
    glGenRenderbuffers(1, &d->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, d->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, d->rbo);

    // Build the texture for conventional rendering
    glGenTextures(1, &d->tex);
    glBindTexture(GL_TEXTURE_2D, d->tex);
    glTexImage2D(GL_TEXTURE_2D,
                 0,                 // level
                 GL_RED,            // internal format
                 size,              // width
                 size,              // height
                 0,                 // border
                 GL_RED,            // format
                 GL_UNSIGNED_INT,   // type
                 NULL);             // data
    setup_texture();

    // Build and link the AO shader programs
    d->vs = shader_build(AO_DEPTH_VS_SRC, GL_VERTEX_SHADER);
    d->fs = shader_build(AO_DEPTH_FS_SRC, GL_FRAGMENT_SHADER);
    d->prog = shader_link_vf(d->vs, d->fs);

    SHADER_GET_UNIFORM_LOC(d, model);
    SHADER_GET_UNIFORM_LOC(d, view);
}

static void ao_depth_deinit(ao_depth_t* d) {
    glDeleteFramebuffers(1, &d->fbo);
    glDeleteRenderbuffers(1, &d->rbo);
    glDeleteTextures(1, &d->tex);
    glDeleteShader(d->vs);
    glDeleteShader(d->fs);
    glDeleteShader(d->prog);
}

static void ao_depth_render(ao_depth_t* d, model_t* model, camera_t* camera) {
    glBindFramebuffer(GL_FRAMEBUFFER, d->fbo);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(d->prog);
    glBindVertexArray(model->vao);

    CAMERA_UNIFORM_MAT(d, model);
    CAMERA_UNIFORM_MAT(d, view);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         d->tex, 0);
    GLenum draw_buf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &draw_buf);

    check_framebuffer();
    log_gl_error();

    glViewport(0, 0, d->size, d->size);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, model->tri_count * 3, GL_UNSIGNED_INT, NULL);

    log_gl_error();
}

static void ao_depth_save_bitmap(ao_depth_t* d, const char* filename) {
    FILE* out = fopen(filename, "w");
    if (!out) {
        log_error_and_abort("Could not open '%s'", filename);
    }

    // Read back the texture from the GPU
    glBindTexture(GL_TEXTURE_2D, d->tex);
    GLuint* pixels = malloc(d->size * d->size * sizeof(GLuint));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_INT, pixels);

    // Write the image
    bitmap_write_header(out, d->size, d->size);
    bitmap_write_depth(out, d->size, d->size, pixels);

    free(pixels);
    fclose(out);
    return;
}

////////////////////////////////////////////////////////////////////////////////

static const GLchar* AO_VOL_VS_SRC = GLSL(330,
layout(location=0) in vec2 pos;
layout(location=1) in vec3 xyz_in;

out vec3 xyz;

void main() {
    gl_Position = (vec4(pos.xy, 0.0f, 1.0f) - 0.5f) * 2.0f;
    xyz = xyz_in;
}
);

static const GLchar* AO_VOL_FS_SRC = GLSL(330,
in vec3 xyz;
out vec4 out_color;

uniform mat4 view;
uniform sampler2D depth;
uniform sampler2D prev;

void main() {
    // This is the target XYZ position, projected into view coordinates
    vec4 pt = view * vec4(xyz, 0.0f);

    // Here are the target and rendered Z positions, which we can compare
    // to decide whether this particular ray made it through.
    float tz = texture(depth, pt.xy / 2.0f + 0.5f).x * 2.0f - 1.0f;
    float pz = pt.z;

    // This is the actual ray's direction, which we accumulate
    vec4 ray = inverse(view) * vec4(0.0f, 0.0f, 1.0f, 0.0f);

    // This is the previous accumulator value
    vec4 prev = texelFetch(prev, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
    if (tz == -1.0f || pz >= tz) {
        out_color = prev + vec4(ray.xyz, 1.0f);
    } else {
        out_color = prev;
    }
}
);

static void ao_vol_init(ao_vol_t* v, unsigned logsize) {
    if (logsize & 1) {
        log_error_and_abort("vol_logsize (%u) must be divisible by 2",
                             logsize);
    }
    //  Generate and bind a framebuffer
    glGenFramebuffers(1, &v->fbo);

    // Generate and configure the volumetric textures
    glGenTextures(2, v->tex);
    v->size = 1 << (logsize / 2 * 3);
    for (unsigned i=0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, v->tex[i]);
        glTexImage2D(GL_TEXTURE_2D,
                     0,             // level
                     GL_RGBA32F,    // internal format
                     v->size,       // width
                     v->size,       // height
                     0,             // border
                     GL_RGBA,       // format
                     GL_FLOAT,      // type
                     NULL);         // data
        setup_texture();
    }
    v->pingpong = 0;

    // Build and link the shader program
    v->vs = shader_build(AO_VOL_VS_SRC, GL_VERTEX_SHADER);
    v->fs = shader_build(AO_VOL_FS_SRC, GL_FRAGMENT_SHADER);
    v->prog = shader_link_vf(v->vs, v->fs);
    glUseProgram(v->prog);
    SHADER_GET_UNIFORM_LOC(v, view);
    SHADER_GET_UNIFORM_LOC(v, depth);
    SHADER_GET_UNIFORM_LOC(v, prev);

    /*  We build a set of square tiles in X and Y, each representing a
        single Z slice of the model, then upload this data to the GPU. */
    v->tri_count = 2 * (1 << logsize);
    const size_t bytes = sizeof(float)
            * v->tri_count
            * 3     // vertices per triangle
            * 5;    // data per vertex (x, y, vx, vy, vz)
    float* buf = malloc(bytes);
    float* ptr = buf;
    const unsigned tiles_per_edge = 1 << (logsize / 2);
    for (unsigned i=0; i < tiles_per_edge; ++i) {
        // xmin, xmax, ymin, ymax are in screen space (0 - 1 range)
        const float xmin =  i      / (float)tiles_per_edge;
        const float xmax = (i + 1) / (float)tiles_per_edge;
        for (unsigned j=0; j < tiles_per_edge; ++j) {
            const float ymin =  j      / (float)tiles_per_edge;
            const float ymax = (j + 1) / (float)tiles_per_edge;

            // vx, vy, and vz are scaled between -1 and 1
            const float vz = ((i * tiles_per_edge) + j) /
                             (float)((1 << logsize) - 1) * 2.0f - 1.0f;

            /* Vertices are as follows:
             *  2----3
             *  |    |
             *  0----1  */
            const float vs[4][5] = {
            //    sx     sy      vx      vy    vz
                {xmin,  ymin,   -1.0,   -1.0,  vz},
                {xmax,  ymin,    1.0,   -1.0,  vz},
                {xmin,  ymax,   -1.0,    1.0,  vz},
                {xmax,  ymax,    1.0,    1.0,  vz}};
            const unsigned triangles[2][3] = {{0, 1, 2}, {2, 1, 3}};
            for (unsigned t=0; t < 2; ++t) {
                for (unsigned v=0; v < 3; ++v) {
                    memcpy(ptr, vs[triangles[t][v]], 5 * sizeof(float));
                    ptr += 5;
                }
            }
        }
    }
    glGenBuffers(1, &v->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, v->vbo);
    glBufferData(GL_ARRAY_BUFFER, bytes, buf, GL_STATIC_DRAW);
    log_gl_error();

    glGenVertexArrays(1, &v->vao);
    glBindVertexArray(v->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(2 * sizeof(float)));
    log_gl_error();

    free(buf);
}

static void ao_vol_deinit(ao_vol_t* v) {
    glDeleteTextures(2, v->tex);
    glDeleteFramebuffers(1, &v->fbo);
    glDeleteShader(v->vs);
    glDeleteShader(v->fs);
    glDeleteShader(v->prog);
    glDeleteVertexArrays(1, &v->vao);
    glDeleteBuffers(1, &v->vbo);
}

static void ao_vol_render(ao_vol_t* v, GLuint depth, camera_t* camera) {
    glBindFramebuffer(GL_FRAMEBUFFER, v->fbo);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(v->prog);
    glBindVertexArray(v->vao);
    CAMERA_UNIFORM_MAT(v, view);

    // v->tex[v->pingpong] was the last render target, so we
    // use it as a texture and render into the other texture.
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         v->tex[!v->pingpong], 0);
    GLenum draw_buf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &draw_buf);
    check_framebuffer();
    log_gl_error();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, v->tex[v->pingpong]);
    glUniform1i(v->u_prev, 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, depth);
    glUniform1i(v->u_depth, 1);
    log_gl_error();

    glViewport(0, 0, v->size, v->size);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, v->tri_count * 3);
    log_gl_error();

    // Bookkeeping total number of rays and most recent buffer
    v->rays++;
    v->pingpong = !v->pingpong;
}

static void ao_vol_save_bitmap(ao_vol_t* v, const char* filename) {
    FILE* out = fopen(filename, "w");
    if (!out) {
        log_error_and_abort("Could not open '%s'", filename);
    }

    // Read back the texture from the GPU
    glBindTexture(GL_TEXTURE_2D, v->tex[v->pingpong]);
    float (*pixels)[4] = malloc(v->size * v->size * sizeof(*pixels));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels);

    // Write the image
    bitmap_write_header(out, v->size, v->size);
    bitmap_write_rays(out, v->size, v->size, pixels, v->rays);

    free(pixels);
    fclose(out);
    return;
}

////////////////////////////////////////////////////////////////////////////////

ao_t* ao_new(unsigned depth_size, unsigned vol_logsize) {
    OBJECT_ALLOC(ao);
    ao_depth_init(&ao->depth, depth_size);
    ao_vol_init(&ao->vol, vol_logsize);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return ao;
}

void ao_render(ao_t* ao, model_t* model, camera_t* camera) {
    // Save initial viewport settings
    GLint prev[4];
    glGetIntegerv(GL_VIEWPORT, prev);

    // Make a modified camera that uses the same model matrix,
    // but has an identity view matrix.  We'll later use the
    // view matrix to render the model from different angles.
    camera_t c;
    memcpy(c.model, camera->model, sizeof(c.model));
    mat4_identity(c.view);

    // Render the model from every triangle on an icosphere
    icosphere_t* ico = icosphere_new(2);
    for (unsigned t=0; t < ico->num_ts; ++t) {
        // Find the main vector (which will be Z)
        float center[3] = {0.0f, 0.0f, 0.0f};
        for (unsigned i=0; i < 3; ++i) {
            for (unsigned j=0; j < 3; ++j) {
                center[i] += ico->vs[ico->ts[t][j]][i];
            }
            center[i] /= 3.0f;
        }
        // Find a perpendicular vector (which will be X)
        float perp[3] = {0.0f, 0.0f, 0.0f};
        for (unsigned i=0; i < 3; ++i) {
            perp[i] = center[i] - ico->vs[ico->ts[t][0]][i];
        }

        vec3_normalize(center);
        vec3_normalize(perp);

        float ortho[3];
        vec3_cross(center, perp, ortho);

        for (unsigned i=0; i < 3; ++i) {
            c.view[i][0] = center[i] / 2.0f;
            c.view[i][1] = perp[i]   / 2.0f;
            c.view[i][2] = ortho[i]  / 2.0f;
        }
        ao_depth_render(&ao->depth, model, &c);
        ao_vol_render(&ao->vol, ao->depth.tex, &c);
    }
    (void)ao_depth_save_bitmap;
    (void)ao_vol_save_bitmap;

    // Restore previous viewport settings
    glViewport(prev[0], prev[1], prev[2], prev[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ao_delete(ao_t* ao) {
    ao_depth_deinit(&ao->depth);
    ao_vol_deinit(&ao->vol);
    free(ao);
}
