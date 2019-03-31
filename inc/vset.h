#include "platform.h"

/*  Structure-of-arrays encoding for a red-black tree which
 *  stores a deduplicated set of vertices */
typedef struct vset_ {
    float (*data)[3];

    uint32_t* parent;
    uint32_t (*child)[2];
    uint8_t *color;

    uint32_t root;
    size_t count;
} vset_t;

/*  Constructs a new vset with the given capacity */
vset_t* vset_with_capacity(size_t num_verts);
void vset_delete(vset_t* v);

/*  Inserts a raw-data vertex into the tree.
 *
 *  d must be a pointer to 9 consecutive floats.
 *  Writes triangle indexes to out */
void vset_insert_tri(vset_t* restrict v, const char* restrict d,
                     uint32_t* restrict out);
uint32_t vset_insert(vset_t* restrict v, const float* restrict f);
void vset_validate(vset_t* v);
