#include "camera.h"
#include "icosphere.h"
#include "loader.h"
#include "log.h"
#include "mat.h"
#include "model.h"
#include "object.h"
#include "platform.h"
#include "worker.h"

static void* loader_run(void* loader_);

void loader_wait(loader_t* loader, loader_state_t target) {
    platform_mutex_lock(loader->mutex);
    while (loader->state < target) {
        platform_cond_wait(loader->cond, loader->mutex);
    }
    platform_mutex_unlock(loader->mutex);
}

void loader_next(loader_t* loader, loader_state_t target) {
    platform_mutex_lock(loader->mutex);
    loader->state = target;
    platform_cond_broadcast(loader->cond);
    platform_mutex_unlock(loader->mutex);
}

loader_t* loader_new(const char* filename) {
    OBJECT_ALLOC(loader);
    loader->mutex = platform_mutex_new();
    loader->cond = platform_cond_new();

    loader->filename = filename;
    loader->thread = platform_thread_new(loader_run, loader);
    return loader;
}

/*  Helper type and function to clean up loader data */
typedef enum { MMAP, DYNAMIC } loader_allocation_t;
static void loader_free(const char* data, size_t size,
                        loader_allocation_t t)
{
    switch (t) {
        case MMAP:      platform_munmap(data, size); break;
        case DYNAMIC:   free((void*)data); break;
    }
}

static const char* loader_parse_ascii(const char* data, size_t* size) {
    size_t buf_size = 256;
    size_t buf_count = 0;
    float* buffer = (float*)malloc(sizeof(float) * buf_size);

#define ABORT_IF(cond, msg) \
    if (cond) {             \
        free(buffer);       \
        log_error(msg);     \
        return NULL;        \
    }

#define SKIP_WHILE(cond)                    \
    while (*data && cond(*data)) {          \
        data++;                             \
    }                                       \
    ABORT_IF(*data == 0, "Unexpected file end");

    /*  The most liberal ASCII STL parser:  Ignore everything except
     *  the word 'vertex', then read three floats after each one. */
    const char VERTEX_STR[] = "vertex ";
    while (1) {
        data = strstr(data, VERTEX_STR);
        if (!data) {
            break;
        }

        /* Skip to the first character after 'vertex' */
        data += strlen(VERTEX_STR);

        for (unsigned i=0; i < 3; ++i) {
            SKIP_WHILE(isspace);
            float f;
            const int r = sscanf(data, "%f", &f);
            ABORT_IF(r == 0 || r == EOF, "Failed to parse float");
            if (buf_size == buf_count) {
                buf_size *= 2;
                buffer = (float*)realloc(buffer, buf_size * sizeof(float));
            }
            buffer[buf_count++] = f;

            SKIP_WHILE(!isspace);
        }
    }
    log_trace("Parsed ASCII STL");

    ABORT_IF(buf_count % 9 != 0, "Total vertex count isn't divisible by 9");
    const uint32_t triangle_count = buf_count / 9;
    *size = 84 + 50 * triangle_count;
    char* out = (char*)malloc(*size);

    /*  Copy triangle count into the buffer */
    memcpy(&out[80], &triangle_count, 4);

    /*  Copy all of the raw triangles into the buffer, spaced out
     *  like a binary STL file (so that we can re-use the reader) */
    for (unsigned i=0; i < buf_count / 9; i++)  {
        memcpy(&out[84 + i*50 + 12], &buffer[i*9], 36);
    }
    return out;
}

static void* loader_run(void* loader_) {
    loader_t* loader = (loader_t*)loader_;
    loader_next(loader, LOADER_START);

    /*  Filesize in bytes; needed to munmap file at the end */
    size_t size;

    /*  This magic filename tells us to load a builtin array,
     *  rather than something in the filesystem */
    const char* mapped;
    loader_allocation_t allocation_type;
    if (!strcmp(loader->filename, ":/sphere")) {
        allocation_type = DYNAMIC;
        mapped = icosphere_stl(1, &size);
    } else {
        allocation_type = MMAP;
        mapped = platform_mmap(loader->filename, &size);
    }

    /*  Check that the file was opened. */
    if (!mapped) {
        log_error("Could not open %s", loader->filename);
        loader_next(loader, LOADER_ERROR_NO_FILE);
        loader_free(mapped, size, allocation_type);
        return NULL;
    }

    /*  Check whether this is an ASCII stl.  Some binary STL files
     *  still start with the word 'solid', so we check the file size
     *  as a second heuristic.  */
    bool is_ascii = (size >= 6 && !strncmp("solid ", mapped, 6));
    if (is_ascii && size >= 84) {
        uint32_t tentative_tri_count;
        memcpy(&tentative_tri_count, &mapped[80],
               sizeof(tentative_tri_count));
        if (size == tentative_tri_count * 50 + 84) {
            log_warn("File begins with 'solid' but appears to be "
                     "a binary STL file");
            is_ascii = false;
        }
    }

    /*  Convert from an ASCII STL to a binary STL so that the rest of the
     *  loader can run unobstructed. */
    if (is_ascii) {
        size_t new_size;
        const char* new_mapped = loader_parse_ascii(mapped, &new_size);
        if (new_mapped) {
            loader_free(mapped, size, allocation_type);
            mapped = new_mapped;
            size = new_size;
            allocation_type = DYNAMIC;
        } else {
            loader_next(loader, LOADER_ERROR_BAD_ASCII_STL);
            loader_free(mapped, size, allocation_type);
            return NULL;
        }
    }

    /*  Check whether the file is a valid size. */
    if (size < 84) {
        log_error("File is too small to be an STL (%zu < 84)", size);
        loader_next(loader, LOADER_ERROR_WRONG_SIZE);
        loader_free(mapped, size, allocation_type);
        return NULL;
    }

    /*  Pull the number of triangles from the raw STL data */
    memcpy(&loader->tri_count, &mapped[80],
           sizeof(loader->tri_count));

    /*  Compare the actual file size with the expected size */
    const uint32_t expected_size = loader->tri_count * 50 + 84;
    if (expected_size != size) {
        log_error("Invalid file size for %u triangles (expected %u, got %zu)",
                  loader->tri_count, expected_size, size);
        loader_next(loader, LOADER_ERROR_WRONG_SIZE);
        loader_free(mapped, size, allocation_type);
        return NULL;
    }

    /*  The worker threads deduplicate a subset of the vertices, then
     *  increment loader->count to indicate that they're done. */
    const size_t NUM_WORKERS = 6;
    worker_t workers[NUM_WORKERS];
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        const size_t start = i * loader->tri_count / NUM_WORKERS;
        const size_t end = (i + 1) * loader->tri_count / NUM_WORKERS;

        workers[i].loader = loader;
        workers[i].tri_count = end - start;
        workers[i].stl = (const char (*)[50])&mapped[80 + 4 + 12 + 50 * start];

        worker_start(&workers[i]);
    }

    /*  Wait for all of the worker threads to finish deduplicating vertices */
    platform_mutex_lock(loader->mutex);
    while (loader->count != NUM_WORKERS) {
        platform_cond_wait(loader->cond, loader->mutex);
    }
    platform_mutex_unlock(loader->mutex);
    log_trace("Workers have deduplicated vertices");

    /*  Accumulate the total vertex count, then wait for the OpenGL thread
     *  to allocate the vertex and triangle buffers */
    loader->vert_count = 0;
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        workers[i].tri_offset = loader->vert_count;
        loader->vert_count += workers[i].vert_count;
    }
    log_trace("Got %u vertices (%u triangles)", loader->vert_count,
            loader->tri_count);
    loader_next(loader, LOADER_MODEL_SIZE);

    log_trace("Waiting for buffer...");
    loader_wait(loader, LOADER_GPU_BUFFER);

    /*  Populate GPU pointers, then kick off workers copying triangles */
    size_t tri_offset = 0;
    size_t vert_offset = 0;
    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        workers[i].vertex_buf = &loader->vertex_buf[vert_offset];
        workers[i].index_buf  = &loader->index_buf[tri_offset];
        vert_offset += workers[i].vert_count * 3;
        tri_offset  += workers[i].tri_count  * 3;
    }
    loader_next(loader, LOADER_WORKER_GPU);
    log_trace("Sent buffers to worker threads");

    for (unsigned i=0; i < NUM_WORKERS; ++i) {
        worker_finish(&workers[i]);
    }
    log_trace("Joined worker threads");

    /*  Reduce min / max arrays from worker subprocesses */
    float scale = 0.0f;
    float center[3];
    for (unsigned v=0; v < 3; ++v) {
        for (unsigned i=1; i < NUM_WORKERS; ++i) {
            if (workers[i].max[v] > workers[0].max[v]) {
                workers[0].max[v] = workers[i].max[v];
            }
            if (workers[i].min[v] < workers[0].min[v]) {
                workers[0].min[v] = workers[i].min[v];
            }
        }
        center[v] = (workers[0].max[v] + workers[0].min[v]) / 2.0f;
        const float d = workers[0].max[v] - workers[0].min[v];
        if (d > scale) {
            scale = d;
        }
    }
    /*  Build the model matrix, which positions the model at
     *  [0,0,0] and scales it to fit the standard GL view */
    float t[4][4];
    mat4_translation(center, t);
    float s[4][4];
    mat4_scaling(1.0f / scale, s);
    mat4_mul(t, s, loader->mat);

    /*  Mark the load as done and post an empty event, to make sure that
     *  the main loop wakes up and checks the loader */
    log_trace("Loader thread done");
    loader_next(loader, LOADER_DONE);
    glfwPostEmptyEvent();

    /*  Release any allocated file data */
    loader_free(mapped, size, allocation_type);

    return NULL;
}

void loader_allocate_vbo(loader_t* loader) {
    glGenBuffers(1, &loader->vbo);
    glGenBuffers(1, &loader->ibo);
    glBindBuffer(GL_ARRAY_BUFFER, loader->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, loader->ibo);
    loader_wait(loader, LOADER_MODEL_SIZE);

    /*  Early return if there is an error in the loader;
     *  we leave the buffer allocated so it can be cleaned
     *  up as usual later. */
    if (loader->state >= LOADER_ERROR) {
        return;
    }

    /*  Allocate and map index buffer */
    const size_t ibo_bytes = loader->tri_count * 3 * sizeof(uint32_t);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo_bytes, NULL, GL_STATIC_DRAW);
    loader->index_buf = (uint32_t*)glMapBufferRange(
            GL_ELEMENT_ARRAY_BUFFER, 0, ibo_bytes,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT
                             | GL_MAP_INVALIDATE_BUFFER_BIT
                             | GL_MAP_UNSYNCHRONIZED_BIT);

    /*  Allocate and map vertex buffer */
    const size_t vbo_bytes = loader->vert_count * 3 * sizeof(float);
    glBufferData(GL_ARRAY_BUFFER, vbo_bytes, NULL, GL_STATIC_DRAW);
    loader->vertex_buf = (float*)glMapBufferRange(
            GL_ARRAY_BUFFER, 0, vbo_bytes,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT
                             | GL_MAP_INVALIDATE_BUFFER_BIT
                             | GL_MAP_UNSYNCHRONIZED_BIT);
    loader_next(loader, LOADER_GPU_BUFFER);

    log_trace("Allocated buffer");
}

void loader_finish(loader_t* loader, model_t* model, camera_t* camera) {
    if (!loader->vbo) {
        log_error_and_abort("Invalid loader VBO");
    } else if (!loader->ibo) {
        log_error_and_abort("Invalid loader IBO");
    } else if (!model->vao) {
        log_error_and_abort("Invalid model VAO");
    }

    loader_wait(loader, LOADER_DONE);

    /*  If the loader succeeded, then set up all of the
     *  GL buffers, matrices, etc. */
    if (loader->state == LOADER_DONE) {
        glBindVertexArray(model->vao);

        glBindBuffer(GL_ARRAY_BUFFER, loader->vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, loader->ibo);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        model->vbo = loader->vbo;
        model->ibo = loader->ibo;
        model->tri_count = loader->tri_count;

        camera_set_model_mat(camera, loader->mat);
        log_trace("Copied model from loader");
    } else {
        log_error("Loading failed");
    }
}

void loader_delete(loader_t* loader) {
    if (platform_thread_join(loader->thread)) {
        log_error_and_abort("Failed to join loader thread");
    }
    platform_mutex_delete(loader->mutex);
    platform_cond_delete(loader->cond);
    platform_thread_delete(loader->thread);
    free(loader);
    log_trace("Destroyed loader");
}

const char* loader_error_string(loader_state_t state) {
    switch(state) {
        case LOADER_START:
        case LOADER_MODEL_SIZE:
        case LOADER_GPU_BUFFER:
        case LOADER_WORKER_GPU:
        case LOADER_DONE:
            return "No error";

        case LOADER_ERROR:
            return "Generic error";
        case LOADER_ERROR_NO_FILE:
            return "File not found";
        case LOADER_ERROR_BAD_ASCII_STL:
            return "Failed to parse ASCII stl";
        case LOADER_ERROR_WRONG_SIZE:
            return "File size does not match triangle count";
    }
}
