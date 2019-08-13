#include "log.h"
#include "vset.h"

#define XXH_INLINE_ALL
#include "xxhash/xxhash.h"

vset_t* vset_new() {
    vset_t* v = (vset_t*)calloc(1, sizeof(vset_t));

    //  Allocate bucket data
    v->num_buckets = 128;
    v->buckets = calloc(v->num_buckets, sizeof(*v->buckets));

    //  Allocate node data
    v->data_size = 128;
    v->vert = calloc(v->data_size, sizeof(*v->vert));
    v->data = calloc(v->data_size, sizeof(*v->data));

    return v;
}

void vset_delete(vset_t* v) {
    free(v->vert);
    free(v->data);
    free(v->buckets);
    free(v);
}

#define REALLOC_TO(data_var, size_var, new_size) do {               \
        void* n = calloc(v->size_var * 2, sizeof(*v->data_var));    \
        memcpy(n, v->data_var, v->size_var * sizeof(*v->data_var)); \
        free(v->data_var);                                          \
        v->data_var = n;                                            \
    } while(0);

#define REALLOC_DOUBLE(data_var, size_var) \
    REALLOC_TO(data_var, size_var, (v->size_var * 2))

static uint32_t vset_new_vertex(vset_t* restrict v, const float* restrict f) {
    const uint32_t i = ++v->count;
    if (i == v->data_size) {
        REALLOC_DOUBLE(vert, data_size);
        REALLOC_DOUBLE(data, data_size);
        v->data_size *= 2;
    }
    // Store the new vertex in the vertex data array
    memcpy(v->vert[i], f, sizeof(*v->vert));
    return i;
}

// Doubles the size of the hashset
static void vset_rehash(vset_t* v, size_t new_buckets) {
    REALLOC_TO(buckets, num_buckets, new_buckets);

    const uint32_t new_mask = new_buckets - 1;
    const uint32_t old_mask = v->num_buckets - 1;

    // Walk through all of the data in the existing map, checking to see
    // which items are now mapped to other buckets and moving them to
    // the head of their new bucket's list.
    //
    // Since size is a power of two, we're adding a single bit to the hash,
    // which means about half of the items should move.
    for (unsigned b=0; b < v->num_buckets; ++b) {
       uint32_t* i = &v->buckets[b];
       while (*i) {
           const uint32_t hash = v->data[*i].hash;
           const uint32_t old_bucket = hash & old_mask;
           assert(old_bucket == b);
           const uint32_t new_bucket = hash & new_mask;

           if (old_bucket == new_bucket) {
               i = &v->data[*i].next;
           } else {
               // Store the next link in the original chain
               const uint32_t next = v->data[*i].next;

               // Move this link to the head of the new bucket's chain
               v->data[*i].next = v->buckets[new_bucket];
               v->buckets[new_bucket] = *i;

               // Keep iterating through the previous bucket's chain
               *i = next;
           }
       }
    }
    v->num_buckets = new_buckets;
}

static uint8_t cmp(const float a[3], const float b[3]) {
    for (unsigned i=0; i < 3; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

uint32_t vset_insert(vset_t* restrict v, const float* restrict f) {
    const uint32_t hash = XXH32(f, 12, 0);
    const uint32_t mask = v->num_buckets - 1;

    const uint32_t b = hash & mask;

    // Walk down the chain for this particular hash, looking for
    // the identical value.  If the loop doesn't return early,
    // then we're left pointing at the next link to assign.
    for (uint32_t* i = &v->buckets[b]; *i != 0; i = &v->data[*i].next) {
       if (v->data[*i].hash == hash && cmp(f, v->vert[*i])) {
           return *i;
       }
    }
    // Insert the new vertex into the data array
    const uint32_t index = vset_new_vertex(v, f);
    v->data[index].hash = hash;

    // Insert the new index as the first item in the relevant bucket's chain
    assert(v->data[index].next == 0);
    v->data[index].next = v->buckets[b];
    v->buckets[b] = index;

    // Resize if the load factor gets above 0.5
    if (v->count > v->num_buckets / 2) {
        vset_rehash(v, v->num_buckets * 2);
    }
    return index;
}

void vset_print_stats(vset_t* v) {
    uint32_t max_chain = 0;
    uint32_t total_chain = 0;
    uint32_t chain_count = 0;
    for (unsigned b=0; b < v->num_buckets; ++b) {
        uint32_t chain_length = 0;
        for (uint32_t i=v->buckets[b]; i; i = v->data[i].next) {
            chain_length++;
        }
        if (chain_length > max_chain) {
            max_chain = chain_length;
        }
        total_chain += chain_length;
        chain_count += (chain_length > 0);
    }

    log_trace("vset has %u nodes", v->count);
    log_trace("   size %u", v->num_buckets);
    log_trace("   longest chain %u", max_chain);
    log_trace("   occupied buckets %u", chain_count);
    log_trace("   average chain %f", (float)total_chain / v->num_buckets);
}
