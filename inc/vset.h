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
 *  d must be a pointer to 3 consecutive floats */
uint32_t vset_insert_raw(vset_t* restrict v, const char* restrict d);
uint32_t vset_insert(vset_t* v);
void vset_validate(vset_t* v);
