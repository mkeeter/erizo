#include "log.h"
#include "vset.h"

#define LEFT    0
#define RIGHT   1

#define CHILD(h) (v->child[h])              /* handle to node */
#define DATA(h) (v->data[h])                /* handle to data */
#define BALANCE(h) (v->balance[h])          /* handle to data */
#define ROOT()  (CHILD(0)[LEFT])       /* root handle */

vset_t* vset_new() {
    vset_t* v = (vset_t*)calloc(1, sizeof(vset_t));

    /*  Allocate node data */
    v->size = 128;
    v->data = calloc(v->size, sizeof(*v->data));
    v->child = calloc(v->size, sizeof(*v->child));
    v->balance = calloc(v->size, sizeof(*v->balance));

    /*  Mark the super-root as the 0th handle */
    v->history[0] = 0;

    return v;
}

void vset_delete(vset_t* v) {
    free(v->data);
    free(v->child);
    free(v->balance);
    free(v);
}

static vset_handle_t vset_next(vset_t* v) {
    const vset_handle_t n = ++v->count;
    if (n == v->size) {
#define REALLOC(t) do {                                     \
            void* n = calloc(v->size * 2, sizeof(*v->t));   \
            memcpy(n, v->t, v->size * sizeof(*v->t));       \
            free(v->t);                                     \
            v->t = n;                                       \
        } while(0);
        REALLOC(data);
        REALLOC(child);
        REALLOC(balance);
#undef REALLOC

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

    vset_handle_t const q = CHILD(p)[RIGHT];
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
    vset_handle_t const a = CHILD(p)[LEFT];
    vset_handle_t const b = CHILD(q)[LEFT];
    vset_handle_t const c = CHILD(q)[RIGHT];

    vset_handle_t const r = ptr[-1];

    /*  Many of these operations could potentially mess with the
     *  parent ande child pointer of the 0 element in our data,
     *  which represents an empty leaf.  That's fine, because we
     *  never look for the leaf's parent or children, so it
     *  doesn't matter if they contain invalid data. */
    CHILD(r)[CHILD(r)[0] != p] = q;
    CHILD(q)[LEFT] = p;
    CHILD(q)[RIGHT] = c;
    CHILD(p)[LEFT] = a;
    CHILD(p)[RIGHT] = b;
}

static void vset_rotate_right(vset_t* v, vset_handle_t const* const ptr) {
    assert(ptr);

    vset_handle_t const q = *ptr;
    assert(q);

    vset_handle_t const p = CHILD(q)[LEFT];
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
    vset_handle_t const a = CHILD(p)[LEFT];
    vset_handle_t const b = CHILD(p)[RIGHT];
    vset_handle_t const c = CHILD(q)[RIGHT];

    vset_handle_t const r = ptr[-1];

    CHILD(r)[CHILD(r)[0] != q] = p;
    CHILD(p)[LEFT] = a;
    CHILD(p)[RIGHT] = q;
    CHILD(q)[LEFT] = b;
    CHILD(q)[RIGHT] = c;
}

static void vset_repair(vset_t* v, vset_handle_t const* const ptr) {
    /*  ptr points to a tree in the history that has just gotten taller,
     *  which means that it must be unbalanced or a a leaf node. */
    assert(BALANCE(*ptr) != 0 ||
               (CHILD(*ptr)[LEFT]  == 0 &&
                CHILD(*ptr)[RIGHT] == 0));

    /*  We start by adjusting the balance of its parent. */
    vset_handle_t const y = ptr[-1];
    assert(y);
    if (ptr[0] == CHILD(y)[LEFT]) {
        BALANCE(y)--;
    } else {
        assert(ptr[0] == CHILD(y)[RIGHT]);
        BALANCE(y)++;
    }

    /*  If we have balanced the parent node, then max height
     *  has not changed and we can stop recursing up the tree. */
    if (BALANCE(y) == 0) {
        return;
    }

    /*  If the balance of the parent node has shifted, then one of
     *  its children has gotten taller, and we need to recurse up the tree. */
    else if (BALANCE(y) == +1 || BALANCE(y) == -1) {
        /*  Stop recursing when we hit the root node */
        if (ptr[-1] != ROOT()) {
            vset_repair(v, ptr - 1);
        }
        return;
    }

    /*  Otherwise, we need to handle the various rebalancing operations */
    else if (BALANCE(y) == -2) {
        vset_handle_t const x = CHILD(y)[LEFT];
        assert(ptr[0] == x);

        if (BALANCE(x) == -1) {
            /*
             *  a* is the modified tree, which unbalances x and y
             *         y--          x0
             *         / \         /  \
             *       x-   c  =>  a*   y0
             *      / \              /  \
             *    a*   b            b    c
             */
            vset_rotate_right(v, ptr - 1);
            BALANCE(y) = 0;
            BALANCE(x) = 0;
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
            assert(BALANCE(x) == 1);

            vset_handle_t const w = CHILD(x)[RIGHT];
            assert(w);

            vset_rotate_left(v, ptr);
            assert(w == CHILD(y)[LEFT]);

            vset_rotate_right(v, ptr - 1);
            if (BALANCE(w) == -1) {
                BALANCE(x) =  0;
                BALANCE(y) = +1;
            } else if (BALANCE(w) == 0) {
                BALANCE(x) =  0;
                BALANCE(y) =  0;
            } else if (BALANCE(w) == 1) {
                BALANCE(x) = -1;
                BALANCE(y) =  0;
            }
            BALANCE(w) = 0;
        }
    /* Symmetric case */
    } else if (BALANCE(y) == 2) {
        vset_handle_t const x = CHILD(y)[RIGHT];
        assert(ptr[0] == x);

        if (BALANCE(x) == +1) {
            vset_rotate_left(v, ptr - 1);
            BALANCE(y) = 0;
            BALANCE(x) = 0;
        } else {
            assert(BALANCE(x) == -1);

            vset_handle_t const w = CHILD(x)[LEFT];
            assert(w);

            vset_rotate_right(v, ptr);
            assert(w == CHILD(y)[RIGHT]);

            vset_rotate_left(v, ptr - 1);
            if (BALANCE(w) == 1) {
                BALANCE(x) =  0;
                BALANCE(y) = -1;
            } else if (BALANCE(w) == 0) {
                BALANCE(x) =  0;
                BALANCE(y) =  0;
            } else if (BALANCE(w) == -1) {
                BALANCE(x) = +1;
                BALANCE(y) =  0;
            }
            BALANCE(w) = 0;
        }
    } else {
        log_error_and_abort("Invalid balance %i", BALANCE(y));
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
        if (CHILD(n)[c] == 0) {
            vset_handle_t j = vset_next(v);
            memcpy(DATA(j), f, sizeof(*v->data));
            CHILD(n)[c] = j;

            /*  Push our new node to the end of the history stack */
            *++ptr = j;

            /*  Walk up the tree, adjusting balance as needed */
            vset_repair(v, ptr);

            return j;
        } else {
            n = CHILD(n)[c];
        }
    }

    assert(false);
    return 0;
}

static int32_t vset_validate_recurse(vset_t* v, vset_handle_t n) {
    if (n == 0) {
        return 0;
    }
    const int32_t a = vset_validate_recurse(v, CHILD(n)[LEFT]);
    const int32_t b = vset_validate_recurse(v, CHILD(n)[RIGHT]);

    const int32_t balance = b - a;
    assert(BALANCE(n) == balance);
    assert(balance >= -1);
    assert(balance <=  1);
    (void)balance;

    return 1 + ((a > b) ? a : b);
}

static uint32_t vset_min_depth(vset_t* v, vset_handle_t n) {
    if (n == 0) {
        return 0;
    }
    uint32_t a = vset_min_depth(v, CHILD(n)[0]);
    uint32_t b = vset_min_depth(v, CHILD(n)[1]);
    return 1 + ((a < b) ? a : b);
}

void vset_validate(vset_t* v) {
    /*  Recursively validate the tree */
    int32_t max_depth = vset_validate_recurse(v, ROOT());

    log_trace("vset has %lu nodes", v->count);
    log_trace("   min depth %u", vset_min_depth(v, ROOT()));
    log_trace("   max depth %u", max_depth);
}
