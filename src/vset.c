#include "log.h"
#include "vset.h"

#define LEFT    0
#define RIGHT   1

#define BLACK   0
#define RED     1

vset_t* vset_with_capacity(size_t num_verts) {
    /* Reserve index 0 for unassigned nodes */
    num_verts += 1;

    /*  Work out the max depth for this tree, to decide how long the
     *  history array should be */
    unsigned max_depth = ceil(2.0 * log2(num_verts + 1.0) + 4.0);

    /* We overallocate this struct so that all of the arrays
     * are close together, then set up the pointers so that
     * they point to the right locations within the struct */
    vset_t* v = (vset_t*)calloc(1, sizeof(vset_t) +
            max_depth * sizeof(*v->history) +
            num_verts * (
                sizeof(*v->data) +
                sizeof(*v->child) +
                sizeof(*v->color)));

    /*  Now, do the exciting math to position internal pointers */
    const char* ptr = (const char*)v;

    ptr += sizeof(vset_t);
    v->data = (float(*)[3])ptr;
    ptr += num_verts * sizeof(*v->data);
    v->history = (uint32_t*)ptr;
    ptr += max_depth * sizeof(*v->history);
    v->child = (uint32_t(*)[2])ptr;
    ptr += num_verts * sizeof(*v->child);
    v->color = (uint8_t*)ptr;

    return v;
}

void vset_delete(vset_t* v) {
    free(v);
}

static uint8_t cmp(const float a[3], const float b[3]) {
    for (unsigned i=0; i < 3; ++i) {
        if (a[i] < b[i]) {
            return 0;
        } else if (a[i] > b[i]) {
            return 1;
        }
    }
    return 2;
}

static void vset_rotate_left(vset_t* v, const uint32_t* const ptr) {
    assert(ptr);

    const uint32_t p = *ptr;
    assert(p);

    const uint32_t q = v->child[p][RIGHT];
    assert(q);

    /*
     *         R                 R
     *         |                 |
     *         P                 Q
     *        / \       ==>     / \
     *       A   Q             P   C
     *          / \           / \
     *         B   C         A   B
     */
    const uint32_t a = v->child[p][LEFT];
    const uint32_t b = v->child[q][LEFT];
    const uint32_t c = v->child[q][RIGHT];

    const uint32_t r = ptr[-1];

    /*  Many of these operations could potentially mess with the
     *  parent and child pointer of the 0 element in our data,
     *  which represents an empty leaf.  That's fine, because we
     *  never look for the leaf's parent or children, so it
     *  doesn't matter if they contain invalid data. */
    v->child[r][v->child[r][0] != p] = q;
    v->child[q][LEFT] = p;
    v->child[q][RIGHT] = c;
    v->child[p][LEFT] = a;
    v->child[p][RIGHT] = b;
}

static void vset_rotate_right(vset_t* v, const uint32_t* const ptr) {
    assert(ptr);

    const uint32_t q = *ptr;
    assert(q);

    const uint32_t p = v->child[q][LEFT];
    assert(p);

    /*
     *         R               R
     *         |               |
     *         Q               P
     *        / \    =>       / \
     *       P   C           A   Q
     *      / \                 / \
     *     A   B               B   C
     */
    const uint32_t a = v->child[p][LEFT];
    const uint32_t b = v->child[p][RIGHT];
    const uint32_t c = v->child[q][RIGHT];

    const uint32_t r = ptr[-1];

    v->child[r][v->child[r][0] != q] = p;
    v->child[p][LEFT] = a;
    v->child[p][RIGHT] = q;
    v->child[q][LEFT] = b;
    v->child[q][RIGHT] = c;
}

static void vset_repair(vset_t* v, const uint32_t* const ptr) {
    assert(ptr);

    uint32_t n = *ptr;
    assert(n);
    assert(v->color[n] == RED);

    /*  The root node must be black */
    uint32_t p = ptr[-1];
    if (!p) {
        v->color[n] = BLACK;
        return;
    }

    /*  Red below black is fine */
    if (v->color[p] == BLACK) {
        return;
    }

    /*  The grandparent must exist, because otherwise the parent
     *  would be the root (and hence would be black) */
    const uint32_t gp = ptr[-2];
    assert(gp);

    /*  Pick out the uncle node, which may be a leaf (0) */
    const uint32_t u = v->child[gp][v->child[gp][0] == p];

    /*  If parent and uncle are red, then color them both black,
     *  make the grand-parent red, and recursively repair up from
     *  the grandparent. */
    if (v->color[u] == RED) {
        v->color[u] = BLACK;
        v->color[p] = BLACK;
        v->color[gp] = RED;
        vset_repair(v, ptr - 2);
        return;
    }

    /*  First stage of fancy repairs */
    if (n == v->child[p][RIGHT] && p == v->child[gp][LEFT]) {
        vset_rotate_left(v, ptr - 1);
        const uint32_t temp = n;
        n = p;
        p = temp;
    } else if (n == v->child[p][LEFT] && p == v->child[gp][RIGHT]) {
        vset_rotate_right(v, ptr - 1);
        const uint32_t temp = n;
        n = p;
        p = temp;
    }

    /*  Second stage of fancy repairs */
    if (v->child[p][LEFT] == n) {
        vset_rotate_right(v, ptr - 2);
    } else {
        vset_rotate_left(v, ptr - 2);
    }
    v->color[p] = BLACK;
    v->color[gp] = RED;
}

uint32_t vset_insert(vset_t* restrict v, const float* restrict f) {
    /*  If the tree is empty, then insert a single black node
     *  (with no children or parent).  The root is stored implicitly
     *  as the LEFT child of the empty node. */
    if (!v->child[0][LEFT]) {
        assert(v->count == 0);

        const size_t n = ++v->count;
        memcpy(v->data[n], f, sizeof(*v->data));
        v->child[0][LEFT] = n;
        return n;
    }

    uint32_t n = v->child[0][LEFT];
    uint32_t* ptr = v->history;
    while (true) {
        /*  Record the current node in the history */
        *++ptr = n;

        /*  If we find the same vertex, then return immediately */
        const uint8_t c = cmp(v->data[n], f);
        if (c == 2) {
            return n;
        }
        /* If we've reached a leaf node, then insert a new
         * node and exit. */
        if (v->child[n][c] == 0) {
            uint32_t j = ++v->count;
            memcpy(v->data[j], f, sizeof(*v->data));
            v->child[n][c] = j;

            v->color[j] = RED;

            /*  Push our new node to the end of the history stack */
            *++ptr = j;

            /*  Handle tree rebalancing */
            vset_repair(v, ptr);
            return j;
        } else {
            n = v->child[n][c];
        }
    }

    assert(false);
    return 0;
}

static uint32_t vset_validate_recurse(vset_t* v, uint32_t n) {
    if (v->color[n] == RED) {
        assert(v->color[v->child[n][0]] == BLACK);
        assert(v->color[v->child[n][1]] == BLACK);
    }

    /*  If this is the leaf node, then there's one black node in the path */
    if (n == 0) {
        return 1;
    }
    const uint32_t a = vset_validate_recurse(v, v->child[n][0]);

    assert(a == vset_validate_recurse(v, v->child[n][1]));
    return a + (v->color[n] == BLACK);
}

static uint32_t vset_min_depth(vset_t* v, uint32_t n) {
    if (n == 0) {
        return 1;
    }
    const uint32_t a = vset_min_depth(v, v->child[n][0]);
    const uint32_t b = vset_min_depth(v, v->child[n][1]);
    return 1 + ((a < b) ? a : b);
}

static uint32_t vset_max_depth(vset_t* v, uint32_t n) {
    if (n == 0) {
        return 1;
    }
    const uint32_t a = vset_max_depth(v, v->child[n][0]);
    const uint32_t b = vset_max_depth(v, v->child[n][1]);
    return 1 + ((a > b) ? a : b);
}

void vset_validate(vset_t* v) {
    /*  Leaf nodes must be black */
    assert(v->color[0] == BLACK);

    /*  Root node must be black */
    assert(v->color[v->child[0][0]] == BLACK);

    /*  Recursively validate the tree */
    vset_validate_recurse(v, v->child[0][LEFT]);

    log_trace("vset has %lu nodes", v->count);
    log_trace("   min depth %u", vset_min_depth(v, v->child[0][LEFT]));
    log_trace("   max depth %u", vset_max_depth(v, v->child[0][LEFT]));
}
