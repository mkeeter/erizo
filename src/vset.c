#include "log.h"
#include "vset.h"

#define LEFT    0
#define RIGHT   1

#define BLACK   0
#define RED     1

vset_t* vset_with_capacity(size_t num_verts) {
    vset_t* v = (vset_t*)malloc(sizeof(vset_t));

    /* Reserve index 0 for unassigned nodes */
    num_verts += 1;

    /*  Allocate all of the data!
     *  By default, nodes are BLACK with null parents and leaf children;
     *  the data array is left unallocated. */
    v->data   =    (float(*)[3])malloc(num_verts * sizeof(*v->data));
    v->parent =      (uint32_t*)calloc(num_verts,  sizeof(*v->parent));
    v->child  = (uint32_t(*)[2])calloc(num_verts,  sizeof(*v->child));
    v->color  =       (uint8_t*)calloc(num_verts,  sizeof(*v->color));
    v->count = 0;
    v->root = 0;

    return v;
}

void vset_delete(vset_t* v) {
    free(v->data);
    free(v->parent);
    free(v->parent);
    free(v->color);
    free(v);
}

uint32_t vset_insert_raw(vset_t* v, const char* data) {
    assert(v);
    assert(data);

    float f[3];
    memcpy(f, data, sizeof(f));
    return vset_insert(v, f);
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

static void vset_rotate_left(vset_t* v, uint32_t p) {
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

    const uint32_t r = v->parent[p];

    /*  Many of these operations could potentially mess with the
     *  parent and child pointer of the 0 element in our data,
     *  which represents an empty leaf.  That's fine, because we
     *  never look for the leaf's parent or children, so it
     *  doesn't matter if they contain invalid data. */
    v->child[r][v->child[r][0] != p] = q;
    v->parent[q] = r;

    v->child[q][LEFT] = p;
    v->parent[p] = q;

    v->child[q][RIGHT] = c;
    v->parent[c] = q;

    v->child[p][LEFT] = a;
    v->parent[a] = p;

    v->child[p][RIGHT] = b;
    v->parent[b] = p;

    /*  Swap the root pointer as needed */
    if (v->root == p) {
        v->root = q;
    }
}

static void vset_rotate_right(vset_t* v, uint32_t q) {
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

    const uint32_t r = v->parent[q];

    v->child[r][v->child[r][0] != q] = p;
    v->parent[p] = r;

    v->child[p][LEFT] = a;
    v->parent[a] = p;

    v->child[p][RIGHT] = q;
    v->parent[q] = p;

    v->child[q][LEFT] = b;
    v->parent[b] = q;

    v->child[q][RIGHT] = c;
    v->parent[c] = q;

    /*  Swap the root pointer as needed */
    if (v->root == q) {
        v->root = p;
    }
}

static void vset_repair(vset_t* v, uint32_t i) {
    assert(v->color[i] == RED);
    assert(i);

    /*  The root node must be black */
    uint32_t p = v->parent[i];
    if (!p) {
        v->color[i] = BLACK;
        return;
    }

    /*  Red below black is fine */
    if (v->color[p] == BLACK) {
        return;
    }

    /*  The grandparent must exist, because otherwise the parent
     *  would be the root (and hence would be black) */
    const uint32_t gp = v->parent[p];
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
        vset_repair(v, gp);
        return;
    }

    /*  First stage of fancy repairs */
    if (i == v->child[p][RIGHT] && p == v->child[gp][LEFT]) {
        vset_rotate_left(v, p);
        i = v->child[i][LEFT];
        p = v->parent[i];
    } else if (i == v->child[p][LEFT] && p == v->child[gp][RIGHT]) {
        vset_rotate_right(v, p);
        i = v->child[i][RIGHT];
        p = v->parent[i];
    }

    /*  Second stage of fancy repairs */
    if (v->child[p][LEFT] == i) {
        vset_rotate_right(v, gp);
    } else {
        vset_rotate_left(v, gp);
    }
    v->color[p] = BLACK;
    v->color[gp] = RED;
}

uint32_t vset_insert(vset_t* v, const float f[3]) {
    /*  If the tree is empty, then insert a single black node
     *  (with no children or parent) */
    if (!v->root) {
        assert(v->count == 0);

        const size_t i = ++v->count;
        v->root = i;
        memcpy(v->data[i], f, sizeof(*v->data));
        return i;
    }

    uint32_t i = v->root;
    while (true) {
        /*  If we find the same vertex, then return immediately */
        const uint8_t c = cmp(v->data[i], f);
        if (c == 2) {
            return i;
        }
        /* If we've reached a leaf node, then insert a new
         * node and exit. */
        if (v->child[i][c] == 0) {
            const size_t j = ++v->count;
            v->child[i][c] = j;

            v->parent[j] = i;
            v->color[j] = RED;
            memcpy(v->data[j], f, sizeof(*v->data));

            /*  Handle tree rebalancing */
            vset_repair(v, j);
            return j;
        } else {
            i = v->child[i][c];
        }
    }

    assert(false);
    return 0;
}

static uint32_t vset_validate_recurse(vset_t* v, uint32_t i) {
    if (v->color[i] == RED) {
        assert(v->color[v->child[i][0]] == BLACK);
        assert(v->color[v->child[i][1]] == BLACK);
    }

    /*  If this is the leaf node, then there's one black node in the path */
    if (i == 0) {
        return 1;
    }
    const uint32_t a = vset_validate_recurse(v, v->child[i][0]);

    assert(a == vset_validate_recurse(v, v->child[i][1]));
    return a + (v->color[i] == BLACK);
}

static uint32_t vset_min_depth(vset_t* v, uint32_t i) {
    if (i == 0) {
        return 1;
    }
    const uint32_t a = vset_min_depth(v, v->child[i][0]);
    const uint32_t b = vset_min_depth(v, v->child[i][1]);
    return 1 + ((a < b) ? a : b);
}

static uint32_t vset_max_depth(vset_t* v, uint32_t i) {
    if (i == 0) {
        return 1;
    }
    const uint32_t a = vset_max_depth(v, v->child[i][0]);
    const uint32_t b = vset_max_depth(v, v->child[i][1]);
    return 1 + ((a > b) ? a : b);
}

void vset_validate(vset_t* v) {
    /*  Leaf nodes must be black */
    assert(v->color[0] == BLACK);

    /*  Root node must be black */
    assert(v->color[v->root] == BLACK);

    /*  Recursively validate the tree */
    vset_validate_recurse(v, v->root);

    log_trace("vset has %lu nodes", v->count);
    log_trace("   min depth %u", vset_min_depth(v, v->root));
    log_trace("   max depth %u", vset_max_depth(v, v->root));
}
