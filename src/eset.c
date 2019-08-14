#include "log.h"
#include "eset.h"

#define XXH_INLINE_ALL
#include "xxhash/xxhash.h"

eset_t* eset_new() {
    eset_t* e = (eset_t*)calloc(1, sizeof(eset_t));

    //  Allocate bucket data
    e->num_buckets = 128;
    e->buckets = calloc(e->num_buckets, sizeof(*e->buckets));

    //  Allocate node data
    e->data_size = 128;
    e->edge = calloc(e->data_size, sizeof(*e->edge));
    e->data = calloc(e->data_size, sizeof(*e->data));

    return e;
}

void eset_delete(eset_t* e) {
    free(e->edge);
    free(e->data);
    free(e->buckets);
    free(e);
}

#define REALLOC_TO(data_var, size_var, new_size) do {               \
        void* n = calloc(e->size_var * 2, sizeof(*e->data_var));    \
        memcpy(n, e->data_var, e->size_var * sizeof(*e->data_var)); \
        free(e->data_var);                                          \
        e->data_var = n;                                            \
    } while(0);

#define REALLOC_DOUBLE(data_var, size_var) \
    REALLOC_TO(data_var, size_var, (e->size_var * 2))

static uint32_t eset_new_edge(eset_t* restrict e, eset_edge_t g, uint32_t tri, uint32_t sub) {
    const uint32_t i = ++e->count;
    if (i == e->data_size) {
        REALLOC_DOUBLE(edge, data_size);
        REALLOC_DOUBLE(data, data_size);
        e->data_size *= 2;
    }
    // Store the new vertex in the vertex data array
    e->edge[i].edge = g;
    e->edge[i].tri = tri;
    e->edge[i].sub = sub;
    return i;
}

// Doubles the size of the hashset
static void eset_rehash(eset_t* e, size_t new_buckets) {
    REALLOC_TO(buckets, num_buckets, new_buckets);

    const uint32_t new_mask = new_buckets - 1;
    const uint32_t old_mask = e->num_buckets - 1;

    // Walk through all of the data in the existing map, checking to see
    // which items are now mapped to other buckets and moving them to
    // the head of their new bucket's list.
    //
    // Since size is a power of two, we're adding a single bit to the hash,
    // which means about half of the items should move.
    for (unsigned b=0; b < e->num_buckets; ++b) {
       uint32_t* i = &e->buckets[b];
       while (*i) {
           const uint32_t hash = e->data[*i].hash;
           const uint32_t old_bucket = hash & old_mask;
           assert(old_bucket == b);
           const uint32_t new_bucket = hash & new_mask;

           if (old_bucket == new_bucket) {
               i = &e->data[*i].next;
           } else {
               // Store the next link in the original chain
               const uint32_t next = e->data[*i].next;

               // Move this link to the head of the new bucket's chain
               e->data[*i].next = e->buckets[new_bucket];
               e->buckets[new_bucket] = *i;

               // Keep iterating through the previous bucket's chain
               *i = next;
           }
       }
    }
    e->num_buckets = new_buckets;
}

static uint8_t cmp(eset_edge_t u, eset_edge_t v) {
    return u.a == v.a && u.b == v.b;
}

uint32_t eset_find(eset_t* restrict e, eset_edge_t g) {
    const uint32_t hash = XXH32(&g, sizeof(g), 0);
    const uint32_t mask = e->num_buckets - 1;

    const uint32_t b = hash & mask;

    // Walk down the chain for this particular hash, looking for
    // the identical value.  If the loop doesn't return early,
    // then we're left pointing at the next link to assign.
    for (uint32_t*i = &e->buckets[b]; *i != 0; i = &e->data[*i].next) {
       if (e->data[*i].hash == hash && cmp(g, e->edge[*i].edge)) {
           return *i;
       }
    }
    return 0;
}

void eset_insert(eset_t* restrict e, eset_edge_t g, uint32_t tri, uint32_t sub) {
    const uint32_t hash = XXH32(&g, sizeof(g), 0);
    const uint32_t mask = e->num_buckets - 1;

    const uint32_t b = hash & mask;

    // Walk down the chain for this particular hash, looking for
    // the identical value.  If the loop doesn't return early,
    // then we're left pointing at the next link to assign.
    for (uint32_t*i = &e->buckets[b]; *i != 0; i = &e->data[*i].next) {
       if (e->data[*i].hash == hash && cmp(g, e->edge[*i].edge)) {
           return;
       }
    }
    // Insert the new vertex into the data array
    const uint32_t index = eset_new_edge(e, g, tri, sub);
    e->data[index].hash = hash;

    // Insert the new index as the first item in the relevant bucket's chain
    assert(e->data[index].next == 0);
    e->data[index].next = e->buckets[b];
    e->buckets[b] = index;

    // Resize if the load factor gets above 0.5
    if (e->count > e->num_buckets / 2) {
        eset_rehash(e, e->num_buckets * 2);
    }
}

void eset_print_stats(eset_t* e) {
    uint32_t max_chain = 0;
    uint32_t total_chain = 0;
    uint32_t chain_count = 0;
    for (unsigned b=0; b < e->num_buckets; ++b) {
        uint32_t chain_length = 0;
        for (uint32_t i=e->buckets[b]; i; i = e->data[i].next) {
            chain_length++;
        }
        if (chain_length > max_chain) {
            max_chain = chain_length;
        }
        total_chain += chain_length;
        chain_count += (chain_length > 0);
    }

    log_trace("eset has %u nodes", e->count);
    log_trace("   size %u", e->num_buckets);
    log_trace("   longest chain %u", max_chain);
    log_trace("   occupied buckets %u", chain_count);
    log_trace("   average chain %f", (float)total_chain / e->num_buckets);
}

