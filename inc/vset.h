#include "platform.h"

/*  Single node in the vset tree */
typedef struct vset_node_ {
    uint32_t child[2];
    int8_t balance;
} vset_node_t;

/*  Tree which stores a deduplicated set of vertices */
typedef struct vset_ {
    float (*data)[3];

    uint32_t* history;
    vset_node_t* node;

    size_t count;
} vset_t;

/*  Constructs a new vset with the given capacity */
vset_t* vset_with_capacity(size_t num_verts);
void vset_delete(vset_t* v);

/*  Inserts a vertex (three floats) into the set, returning an index */
uint32_t vset_insert(vset_t* restrict v, const float* restrict f);

/*  Validates that the red-black tree properties are respected */
void vset_validate(vset_t* v);
