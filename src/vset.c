#include "log.h"
#include "vset.h"

#define LEFT    0
#define RIGHT   1

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
            num_verts * (sizeof(*v->data) + sizeof(*v->node)));

    /*  Now, do the exciting math to position internal pointers */
    const char* ptr = (const char*)v;

    ptr += sizeof(vset_t);
    v->data = (float(*)[3])ptr;
    ptr += num_verts * sizeof(*v->data);
    v->history = (uint32_t*)ptr;
    ptr += max_depth * sizeof(*v->history);
    v->node = (vset_node_t*)ptr;

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

    const uint32_t q = v->node[p].child[RIGHT];
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
    const uint32_t a = v->node[p].child[LEFT];
    const uint32_t b = v->node[q].child[LEFT];
    const uint32_t c = v->node[q].child[RIGHT];

    const uint32_t r = ptr[-1];

    /*  Many of these operations could potentially mess with the
     *  parent ande child pointer of the 0 element in our data,
     *  which represents an empty leaf.  That's fine, because we
     *  never look for the leaf's parent or children, so it
     *  doesn't matter if they contain invalid data. */
    v->node[r].child[v->node[r].child[0] != p] = q;
    v->node[q].child[LEFT] = p;
    v->node[q].child[RIGHT] = c;
    v->node[p].child[LEFT] = a;
    v->node[p].child[RIGHT] = b;
}

static void vset_rotate_right(vset_t* v, const uint32_t* const ptr) {
    assert(ptr);

    const uint32_t q = *ptr;
    assert(q);

    const uint32_t p = v->node[q].child[LEFT];
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
    const uint32_t a = v->node[p].child[LEFT];
    const uint32_t b = v->node[p].child[RIGHT];
    const uint32_t c = v->node[q].child[RIGHT];

    const uint32_t r = ptr[-1];

    v->node[r].child[v->node[r].child[0] != q] = p;
    v->node[p].child[LEFT] = a;
    v->node[p].child[RIGHT] = q;
    v->node[q].child[LEFT] = b;
    v->node[q].child[RIGHT] = c;
}

#define VSET_ROOT(v) v->node[0].child[LEFT]

static void vset_repair(vset_t* v, const uint32_t* const ptr) {
    /*  ptr points to a tree in the history that has just gotten taller,
     *  which means that it must be unbalanced or a a leaf node. */
    assert(v->node[*ptr].balance != 0 ||
               (v->node[*ptr].child[LEFT] == 0 &&
                v->node[*ptr].child[RIGHT] == 0));

    /*  We start by adjusting the balance of its parent. */
    const uint32_t y = ptr[-1];
    assert(y);
    if (ptr[0] == v->node[y].child[LEFT]) {
        v->node[y].balance--;
    } else {
        assert(ptr[0] == v->node[y].child[RIGHT]);
        v->node[y].balance++;
    }

    /*  If we have balanced the parent node, then max height
     *  has not changed and we can stop recursing up the tree. */
    if (v->node[y].balance == 0) {
        return;
    }

    /*  If the balance of the parent node has shifted, then one of
     *  its children has gotten taller, and we need to recurse up the tree. */
    else if (v->node[y].balance == 1 || v->node[y].balance == -1) {
        /*  Stop recursing when we hit the root node */
        if (ptr[-1] != VSET_ROOT(v)) {
            vset_repair(v, ptr - 1);
        }
        return;
    }

    /*  Otherwise, we need to handle the various rebalancing operations */
    else if (v->node[y].balance == -2) {
        const uint32_t x = v->node[y].child[LEFT];
        assert(ptr[0] == x);

        if (v->node[x].balance == -1) {
            /*
             *  a* is the modified tree, which unbalances x and y
             *         y--          x0
             *         / \         /  \
             *       x-   c  =>  a*   y0
             *      / \              /  \
             *    a*   b            b    c
             */
            vset_rotate_right(v, ptr - 1);
            v->node[y].balance = 0;
            v->node[x].balance = 0;
        } else {
            /*
             *         y--              y--            w0
             *         / \             / \            /  \
             *       x+   d           w   d          x    y
             *      / \       ==>    / \      ==>   / \  / \
             *    a    w            x   c           a b  c d
             *        / \          / \
             *       b   c        a   b
             */
            assert(v->node[x].balance == 1);

            const uint32_t w = v->node[x].child[RIGHT];
            assert(w);

            vset_rotate_left(v, ptr);
            assert(w == v->node[y].child[LEFT]);

            vset_rotate_right(v, ptr - 1);
            if (v->node[w].balance == -1) {
                v->node[x].balance =  0;
                v->node[y].balance = +1;
            } else if (v->node[w].balance == 0) {
                v->node[x].balance =  0;
                v->node[y].balance =  0;
            } else if (v->node[w].balance == 1) {
                v->node[x].balance = -1;
                v->node[y].balance =  0;
            }
            v->node[w].balance = 0;
        }
    /* Symmetric case */
    } else if (v->node[y].balance == 2) {
        const uint32_t x = v->node[y].child[RIGHT];
        assert(ptr[0] == x);

        if (v->node[x].balance == +1) {
            vset_rotate_left(v, ptr - 1);
            v->node[y].balance = 0;
            v->node[x].balance = 0;
        } else {
            assert(v->node[x].balance == -1);

            const uint32_t w = v->node[x].child[LEFT];
            assert(w);

            vset_rotate_right(v, ptr);
            assert(w == v->node[y].child[RIGHT]);

            vset_rotate_left(v, ptr - 1);
            if (v->node[w].balance == 1) {
                v->node[x].balance =  0;
                v->node[y].balance = -1;
            } else if (v->node[w].balance == 0) {
                v->node[x].balance =  0;
                v->node[y].balance =  0;
            } else if (v->node[w].balance == -1) {
                v->node[x].balance = +1;
                v->node[y].balance =  0;
            }
            v->node[w].balance = 0;
        }
    } else {
        log_error_and_abort("Invalid balance %i", v->node[y].balance);
    }
}

uint32_t vset_insert(vset_t* restrict v, const float* restrict f) {
    /*  If the tree is empty, then insert a single black node
     *  (with no children or parent).  The root is stored implicitly
     *  as the LEFT child of the empty node. */
    if (!VSET_ROOT(v)) {
        assert(v->count == 0);

        const size_t n = ++v->count;
        memcpy(v->data[n], f, sizeof(*v->data));
        VSET_ROOT(v) = n;
        return n;
    }

    uint32_t n = VSET_ROOT(v);
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
        if (v->node[n].child[c] == 0) {
            uint32_t j = ++v->count;
            memcpy(v->data[j], f, sizeof(*v->data));
            v->node[n].child[c] = j;

            /*  A leaf is perfectly balanced */
            v->node[j].balance = 0;

            /*  Push our new node to the end of the history stack */
            *++ptr = j;

            /*  Walk up the tree, adjusting balance as needed */
            vset_repair(v, ptr);

            return j;
        } else {
            n = v->node[n].child[c];
        }
    }

    assert(false);
    return 0;
}

static int32_t vset_validate_recurse(vset_t* v, uint32_t n) {
    if (n == 0) {
        return 0;
    }
    const int32_t a = vset_validate_recurse(v, v->node[n].child[LEFT]);
    const int32_t b = vset_validate_recurse(v, v->node[n].child[RIGHT]);

    const int32_t balance = b - a;
    assert(v->node[n].balance == balance);
    assert(balance >= -1);
    assert(balance <=  1);
    (void)balance;

    return 1 + ((a > b) ? a : b);
}

static uint32_t vset_min_depth(vset_t* v, uint32_t n) {
    if (n == 0) {
        return 0;
    }
    const uint32_t a = vset_min_depth(v, v->node[n].child[0]);
    const uint32_t b = vset_min_depth(v, v->node[n].child[1]);
    return 1 + ((a < b) ? a : b);
}

void vset_validate(vset_t* v) {
    /*  Leaf nodes must be balanced */
    assert(v->node[0].balance == 0);

    /*  Recursively validate the tree */
    int32_t max_depth = vset_validate_recurse(v, VSET_ROOT(v));

    log_trace("vset has %lu nodes", v->count);
    log_trace("   min depth %u", vset_min_depth(v, VSET_ROOT(v)));
    log_trace("   max depth %u", max_depth);
}
