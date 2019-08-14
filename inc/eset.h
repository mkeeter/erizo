#include "base.h"

/*  Used to refer to nodes within the tree */

typedef struct eset_link_ {
   uint32_t hash;
   uint32_t next;
} eset_link_t;

typedef struct edge_ {
    uint32_t a;
    uint32_t b;
} eset_edge_t;

typedef struct edge_tri {
    eset_edge_t edge;
    uint32_t tri;
    uint32_t sub;
} eset_edge_tri_t;

/*  Tree which stores a deduplicated set of vertices */
typedef struct eset_ {
    eset_edge_tri_t* edge;      /* Raw edge data */
    eset_link_t* data;          /* Separate chaining data */
    size_t data_size;

    uint32_t* buckets;
    uint32_t num_buckets;

    uint32_t count;             /* Number of used nodes */
} eset_t;

/*  Constructs a new eset */
eset_t* eset_new();
void eset_delete(eset_t* v);

/*  Inserts a vertex (three floats) into the set, returning an index */
void eset_insert(eset_t* restrict v, eset_edge_t e, uint32_t tri, uint32_t sub);
uint32_t eset_find(eset_t* restrict v, eset_edge_t e);

/*  Prints statistics about the hashset */
void eset_print_stats(eset_t* v);
