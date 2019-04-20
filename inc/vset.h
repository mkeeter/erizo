#include "base.h"

/*  Used to refer to nodes within the tree */
typedef uint32_t vset_handle_t;

/*  Tree which stores a deduplicated set of vertices */
typedef struct vset_ {
    float (*data)[3];           /* Raw vertex data */
    vset_handle_t (*child)[2];  /* Left and right child indexes */
    int8_t* balance;            /* Node balance values */

    vset_handle_t* history;     /* Used when traversing down the tree */

    size_t size;                /* Maximum available nodes */
    size_t count;               /* Number of used nodes */
} vset_t;

/*  Constructs a new vset with the given capacity */
vset_t* vset_with_capacity(size_t num_verts);
void vset_delete(vset_t* v);

/*  Inserts a vertex (three floats) into the set, returning an index */
uint32_t vset_insert(vset_t* restrict v, const float* restrict f);

/*  Validates that the red-black tree properties are respected */
void vset_validate(vset_t* v);
