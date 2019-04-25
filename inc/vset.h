#include "base.h"

/*  Used to refer to nodes within the tree */

typedef struct vset_link_ {
   uint32_t hash;
   uint32_t next;
} vset_link_t;

/*  Tree which stores a deduplicated set of vertices */
typedef struct vset_ {
    float (*data)[3];           /* Raw vertex data */
    vset_link_t* links;  /* Separate chaining data */

    uint32_t* buckets;
    size_t size;

    size_t count;               /* Number of used nodes */
} vset_t;

/*  Constructs a new vset */
vset_t* vset_new();
void vset_delete(vset_t* v);

/*  Inserts a vertex (three floats) into the set, returning an index */
uint32_t vset_insert(vset_t* restrict v, const float* restrict f);

/*  Prints statistics about the hashset */
void vset_print_stats(vset_t* v);
