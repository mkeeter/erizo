#include "log.h"
#include "vset.h"

#define LEFT    0
#define RIGHT   1

#define NODE(h) (v->node[h])                /* handle to node */
#define DATA(h) (v->data[h])                /* handle to data */
#define ROOT()  (NODE(0).child[LEFT])       /* root handle */

vset_t* vset_with_capacity(size_t num_verts) {
    /* Reserve index 0 for unassigned nodes */
    num_verts += 1;

    vset_t* v = (vset_t*)calloc(1, sizeof(vset_t));

    /*  Allocate node data */
    v->size = 128;
    v->data = calloc(v->size, sizeof(*v->data));
    v->node = calloc(v->size, sizeof(*v->node));

    /*  Work out the max depth for this tree, to decide how long the
     *  history array should be */
    unsigned max_depth = ceil(2.0 * log2(num_verts + 1.0) + 4.0);
    v->history = calloc(max_depth, sizeof(*v->history));

    /*  Mark the super-root as the 0th handle */
    v->history[0] = 0;

    return v;
}

void vset_delete(vset_t* v) {
    free(v->data);
    free(v->node);
    free(v->history);
    free(v);
}

static vset_handle_t vset_next(vset_t* v) {
    const vset_handle_t n = ++v->count;
    if (n == v->size) {
        void* new_data = calloc(v->size * 2, sizeof(*v->data));
        memcpy(new_data, v->data, v->size * sizeof(*v->data));
        free(v->data);
        v->data = new_data;

        void* new_node = calloc(v->size * 2, sizeof(*v->node));
        memcpy(new_node, v->node, v->size * sizeof(*v->node));
        free(v->node);
        v->node = new_node;

        v->size *= 2;
    }
    return n;
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

static void vset_rotate_left(vset_t* v, const vset_handle_t* const ptr) {
    assert(ptr);

    vset_handle_t const p = *ptr;
    assert(p);

    vset_handle_t const q = NODE(p).child[RIGHT];
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
    vset_handle_t const a = NODE(p).child[LEFT];
    vset_handle_t const b = NODE(q).child[LEFT];
    vset_handle_t const c = NODE(q).child[RIGHT];

    vset_handle_t const r = ptr[-1];

    /*  Many of these operations could potentially mess with the
     *  parent ande child pointer of the 0 element in our data,
     *  which represents an empty leaf.  That's fine, because we
     *  never look for the leaf's parent or children, so it
     *  doesn't matter if they contain invalid data. */
    NODE(r).child[NODE(r).child[0] != p] = q;
    NODE(q).child[LEFT] = p;
    NODE(q).child[RIGHT] = c;
    NODE(p).child[LEFT] = a;
    NODE(p).child[RIGHT] = b;
}

static void vset_rotate_right(vset_t* v, vset_handle_t const* const ptr) {
    assert(ptr);

    vset_handle_t const q = *ptr;
    assert(q);

    vset_handle_t const p = NODE(q).child[LEFT];
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
    vset_handle_t const a = NODE(p).child[LEFT];
    vset_handle_t const b = NODE(p).child[RIGHT];
    vset_handle_t const c = NODE(q).child[RIGHT];

    vset_handle_t const r = ptr[-1];

    NODE(r).child[NODE(r).child[0] != q] = p;
    NODE(p).child[LEFT] = a;
    NODE(p).child[RIGHT] = q;
    NODE(q).child[LEFT] = b;
    NODE(q).child[RIGHT] = c;
}

static void vset_repair(vset_t* v, vset_handle_t const* const ptr) {
    /*  ptr points to a tree in the history that has just gotten taller,
     *  which means that it must be unbalanced or a a leaf node. */
    assert(NODE(*ptr).balance != 0 ||
               (NODE(*ptr).child[LEFT]  == 0 &&
                NODE(*ptr).child[RIGHT] == 0));

    /*  We start by adjusting the balance of its parent. */
    vset_handle_t const y = ptr[-1];
    assert(y);
    if (ptr[0] == NODE(y).child[LEFT]) {
        NODE(y).balance--;
    } else {
        assert(ptr[0] == NODE(y).child[RIGHT]);
        NODE(y).balance++;
    }

    /*  If we have balanced the parent node, then max height
     *  has not changed and we can stop recursing up the tree. */
    if (NODE(y).balance == 0) {
        return;
    }

    /*  If the balance of the parent node has shifted, then one of
     *  its children has gotten taller, and we need to recurse up the tree. */
    else if (NODE(y).balance == +1 || NODE(y).balance == -1) {
        /*  Stop recursing when we hit the root node */
        if (ptr[-1] != ROOT()) {
            vset_repair(v, ptr - 1);
        }
        return;
    }

    /*  Otherwise, we need to handle the various rebalancing operations */
    else if (NODE(y).balance == -2) {
        vset_handle_t const x = NODE(y).child[LEFT];
        assert(ptr[0] == x);

        if (NODE(x).balance == -1) {
            /*
             *  a* is the modified tree, which unbalances x and y
             *         y--          x0
             *         / \         /  \
             *       x-   c  =>  a*   y0
             *      / \              /  \
             *    a*   b            b    c
             */
            vset_rotate_right(v, ptr - 1);
            NODE(y).balance = 0;
            NODE(x).balance = 0;
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
            assert(NODE(x).balance == 1);

            vset_handle_t const w = NODE(x).child[RIGHT];
            assert(w);

            vset_rotate_left(v, ptr);
            assert(w == NODE(y).child[LEFT]);

            vset_rotate_right(v, ptr - 1);
            if (NODE(w).balance == -1) {
                NODE(x).balance =  0;
                NODE(y).balance = +1;
            } else if (NODE(w).balance == 0) {
                NODE(x).balance =  0;
                NODE(y).balance =  0;
            } else if (NODE(w).balance == 1) {
                NODE(x).balance = -1;
                NODE(y).balance =  0;
            }
            NODE(w).balance = 0;
        }
    /* Symmetric case */
    } else if (NODE(y).balance == 2) {
        vset_handle_t const x = NODE(y).child[RIGHT];
        assert(ptr[0] == x);

        if (NODE(x).balance == +1) {
            vset_rotate_left(v, ptr - 1);
            NODE(y).balance = 0;
            NODE(x).balance = 0;
        } else {
            assert(NODE(x).balance == -1);

            vset_handle_t const w = NODE(x).child[LEFT];
            assert(w);

            vset_rotate_right(v, ptr);
            assert(w == NODE(y).child[RIGHT]);

            vset_rotate_left(v, ptr - 1);
            if (NODE(w).balance == 1) {
                NODE(x).balance =  0;
                NODE(y).balance = -1;
            } else if (NODE(w).balance == 0) {
                NODE(x).balance =  0;
                NODE(y).balance =  0;
            } else if (NODE(w).balance == -1) {
                NODE(x).balance = +1;
                NODE(y).balance =  0;
            }
            NODE(w).balance = 0;
        }
    } else {
        log_error_and_abort("Invalid balance %i", NODE(y).balance);
    }
}

uint32_t vset_insert(vset_t* restrict v, const float* restrict f) {
    /*  If the tree is empty, then insert a single balanced node
     *  (with no children or parent).  The root is stored implicitly
     *  as the LEFT child of the empty node. */
    if (!ROOT()) {
        assert(v->count == 0);

        const vset_handle_t n = ++v->count;
        memcpy(DATA(n), f, sizeof(*v->data));
        ROOT() = n;
        return n;
    }

    vset_handle_t n = ROOT();
    vset_handle_t* ptr = v->history;
    while (true) {
        /*  Record the current node in the history */
        *++ptr = n;

        /*  If we find the same vertex, then return immediately */
        const uint32_t c = cmp(DATA(n), f);
        if (c == 2) {
            return n;
        }
        /* If we've reached a leaf node, then insert a new
         * node and exit. */
        if (NODE(n).child[c] == 0) {
            vset_handle_t j = vset_next(v);
            memcpy(DATA(j), f, sizeof(*v->data));
            NODE(n).child[c] = j;

            /*  Push our new node to the end of the history stack */
            *++ptr = j;

            /*  Walk up the tree, adjusting balance as needed */
            vset_repair(v, ptr);

            return j;
        } else {
            n = NODE(n).child[c];
        }
    }

    assert(false);
    return 0;
}

static int32_t vset_validate_recurse(vset_t* v, vset_handle_t n) {
    if (n == 0) {
        return 0;
    }
    const int32_t a = vset_validate_recurse(v, NODE(n).child[LEFT]);
    const int32_t b = vset_validate_recurse(v, NODE(n).child[RIGHT]);

    const int32_t balance = b - a;
    assert(NODE(n).balance == balance);
    assert(balance >= -1);
    assert(balance <=  1);
    (void)balance;

    return 1 + ((a > b) ? a : b);
}

static uint32_t vset_min_depth(vset_t* v, vset_handle_t n) {
    if (n == 0) {
        return 0;
    }
    uint32_t a = vset_min_depth(v, NODE(n).child[0]);
    uint32_t b = vset_min_depth(v, NODE(n).child[1]);
    return 1 + ((a < b) ? a : b);
}

void vset_validate(vset_t* v) {
    /*  Recursively validate the tree */
    int32_t max_depth = vset_validate_recurse(v, ROOT());

    log_trace("vset has %lu nodes", v->count);
    log_trace("   min depth %u", vset_min_depth(v, ROOT()));
    log_trace("   max depth %u", max_depth);
}
