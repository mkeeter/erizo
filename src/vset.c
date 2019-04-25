#include "log.h"
#include "vset.h"

#define XXH_INLINE_ALL
#include "xxhash/xxhash.h"

vset_t* vset_new() {
    vset_t* v = (vset_t*)calloc(1, sizeof(vset_t));

    /*  Allocate node data */
    v->size = 128;
    v->data = calloc(v->size, sizeof(*v->data));
    v->buckets = calloc(v->size, sizeof(*v->buckets));
    v->links = calloc(v->size, sizeof(*v->links));

    return v;
}

void vset_delete(vset_t* v) {
    free(v->data);
    free(v->links);
    free(v->buckets);
    free(v);
}

// Doubles the size of the hashset
static void vset_expand(vset_t* v) {
#define REALLOC(t) do {                                     \
        void* n = calloc(v->size * 2, sizeof(*v->t));   \
        memcpy(n, v->t, v->size * sizeof(*v->t));       \
        free(v->t);                                     \
        v->t = n;                                       \
    } while(0);
    REALLOC(data);
    REALLOC(links);
    REALLOC(buckets);
#undef REALLOC
    const uint32_t new_mask = (v->size * 2) - 1;
    const uint32_t old_mask =  v->size - 1;
    for (unsigned b=0; b < v->size; ++b) {
       uint32_t* i = &v->buckets[b];
       while (*i) {
           const uint32_t hash = v->links[*i].hash;
           const uint32_t old_bucket = hash & old_mask;
           assert(old_bucket == b);
           const uint32_t new_bucket = hash & new_mask;

           if (old_bucket == new_bucket) {
               i = &v->links[*i].next;
           } else {
               // Store the next link in the original chain
               const uint32_t next = v->links[*i].next;

               // Move this link to the head of the new bucket's chain
               v->links[*i].next = v->buckets[new_bucket];
               v->buckets[new_bucket] = *i;

               // Keep iterating through the previous bucket's chain
               *i = next;
           }
       }
    }
    v->size *= 2;
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
    const uint32_t mask = v->size - 1;

    // Walk down the chain for this particular hash, looking for
    // the identical value.  If the loop doesn't return early,
    // then we're left pointing at the next link to assign.
    uint32_t* i = &v->buckets[hash & mask];
    while (*i) {
       if (v->links[*i].hash == hash && cmp(f, v->data[*i])) {
           return *i;
       }
       i = &v->links[*i].next;
    }
    // Record the index separately, because vset_expand could change
    // the value of *i if the new item moves to a different bucket.
    const uint32_t index = ++v->count;

    // Record the hash and raw data
    v->links[index].hash = hash;
    memcpy(v->data[index], f, sizeof(*v->data));

    // Link the new data into the chain
    assert(v->links[index].next == 0);
    *i = index;

    // Resize if the load factor gets above 0.5
    if (v->count > v->size / 2) {
        vset_expand(v);
    }
    return index;
}

void vset_print_stats(vset_t* v) {
    uint32_t max_chain = 0;
    uint32_t total_chain = 0;
    uint32_t chain_count = 0;
    for (unsigned b=0; b < v->size; ++b) {
        uint32_t chain_length = 0;
        for (uint32_t i=v->buckets[b]; i; i = v->links[i].next) {
            chain_length++;
        }
        if (chain_length > max_chain) {
            max_chain = chain_length;
        }
        total_chain += chain_length;
        chain_count += (chain_length > 0);
    }

    log_trace("vset has %lu nodes", v->count);
    log_trace("   size %lu", v->size);
    log_trace("   longest chain %u", max_chain);
    log_trace("   occupied buckets %u", chain_count);
    log_trace("   average chain %f", (float)total_chain / v->size);
}
