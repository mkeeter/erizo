#include "ao.h"
#include "object.h"

ao_t* ao_new(unsigned render_size, unsigned vol_size) {
    OBJECT_ALLOC(ao);

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

    return ao;
}

void ao_delete(ao_t* ao) {
    glDeleteFramebuffers(1, &ao->fbo);
    glDeleteRenderbuffers(1, &ao->depth);
    glDeleteTextures(1, &ao->tex);
    glDeleteTextures(2,  ao->vol);
}
