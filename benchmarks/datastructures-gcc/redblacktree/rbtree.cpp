#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "rbtree.h"

#define LDA(a)      *(a)
#define STA(a,v)    *(a) = (v)
#define LDV(a)      (a)
#define STV(a,v)    (a) = (v)
#define LDF(o,f)    ((o)->f)
#define STF(o,f,v)  ((o)->f) = (v)
#define LDNODE(o,f) ((node_t*) (LDF((o),f)))
#define LDC(o,f)    ((intptr_t) (LDF((o),f)))

#define TX_LDA(a)       (*(a))
#define TX_STA(a,v)     (*(a) = v)
#define TX_LDV(a)       (a)
#define TX_STV(a,v)     (a = v)
#define TX_LDF(o,f)     ((o)->f)
#define TX_LDF_P(o,f)   ((o)->f)
#define TX_STF(o,f,v)   ((o)->f = v)
#define TX_STF_P(o,f,v) ((o)->f = v)
#define TX_LDNODE(o,f)  ((node_t*) (TX_LDF_P((o),f)))
#define TX_LDC(o,f)     ((intptr_t) (TX_LDF_P((o),f)))

enum {
    RED   = 0,
    BLACK = 1
};


/*
 * See also:
 * - Doug Lea's j.u.TreeMap
 * - Keir Fraser's rb_stm.c and rb_lock_serialisedwriters.c in libLtx.
 *
 * Following Doug Lea's TreeMap example, we avoid the use of the magic
 * "nil" sentinel pointers.  The sentinel is simply a convenience and
 * is not fundamental to the algorithm.  We forgo the sentinel as
 * it is a source of false+ data conflicts in transactions.  Relatedly,
 * even with locks, use of a nil sentil can result in considerable
 * cache coherency traffic on traditional SMPs.
 */


/* =============================================================================
 * lookup
 * =============================================================================
 */
static node_t*
lookup (rbtree_t* s, int k)
{
    node_t* p = LDNODE(s, root);

    while (p != NULL) {
        int cmp = k - LDF(p, k);
        if (cmp == 0) {
            return p;
        }
        p = ((cmp < 0) ? LDNODE(p, l) : LDNODE(p, r));
    }

    return NULL;
}
#define LOOKUP(set, key)  lookup(set, key)


/*
 * Balancing operations.
 *
 * Implementations of rebalancings during insertion and deletion are
 * slightly different than the CLR version.  Rather than using dummy
 * nilnodes, we use a set of accessors that deal properly with null.  They
 * are used to avoid messiness surrounding nullness checks in the main
 * algorithms.
 *
 * From CLR
 */


/* =============================================================================
 * rotateLeft
 * =============================================================================
 */
static void
rotateLeft (rbtree_t* s, node_t* x )
{
    node_t* r = LDNODE(x, r); /* AKA r, y */
    node_t* rl = LDNODE(r, l);
    STF(x, r, rl);
    if (rl != NULL) {
        STF(rl, p, x);
    }
    /* TODO: compute p = xp = x->p.  Use xp for R-Values in following */
    node_t* xp = LDNODE(x, p);
    STF(r, p, xp);
    if (xp == NULL) {
        STF(s, root, r);
    } else if (LDNODE(xp, l) == x) {
        STF(xp, l, r);
    } else {
        STF(xp, r, r);
    }
    STF(r, l, x);
    STF(x, p, r);
}
#define ROTATE_LEFT(set, node)  rotateLeft(set, node)


/* =============================================================================
 * rotateRight
 * =============================================================================
 */
static void
rotateRight (rbtree_t* s, node_t* x)
{
    node_t* l = LDNODE(x, l); /* AKA l,y */
    node_t* lr = LDNODE(l, r);
    STF(x, l, lr);
    if (lr != NULL) {
        STF(lr, p, x);
    }
    node_t* xp = LDNODE(x, p);
    STF(l, p, xp);
    if (xp == NULL) {
        STF(s, root, l);
    } else if (LDNODE(xp, r) == x) {
        STF(xp, r, l);
    } else {
        STF(xp, l, l);
    }
    STF(l, r, x);
    STF(x, p, l);
}
#define ROTATE_RIGHT(set, node)  rotateRight(set, node)


/* =============================================================================
 * parentOf
 * =============================================================================
 */
static inline node_t*
parentOf (node_t* n)
{
   return (n ? LDNODE(n,p) : NULL);
}
#define PARENT_OF(n) parentOf(n)


/* =============================================================================
 * leftOf
 * =============================================================================
 */
static inline node_t*
leftOf (node_t* n)
{
   return (n ? LDNODE(n, l) : NULL);
}
#define LEFT_OF(n)  leftOf(n)


/* =============================================================================
 * rightOf
 * =============================================================================
 */
static inline node_t*
rightOf (node_t* n)
{
    return (n ? LDNODE(n, r) : NULL);
}
#define RIGHT_OF(n)  rightOf(n)


/* =============================================================================
 * colorOf
 * =============================================================================
 */
static inline int
colorOf (node_t* n)
{
    return (n ? (int)LDC(n, c) : BLACK);
}
#define COLOR_OF(n)  colorOf(n)


/* =============================================================================
 * setColor
 * =============================================================================
 */
static inline void
setColor (node_t* n, int c)
{
    if (n != NULL) {
        STF(n, c, c);
    }
}
#define SET_COLOR(n, c)  setColor(n, c)


/* =============================================================================
 * fixAfterInsertion
 * =============================================================================
 */
static void
fixAfterInsertion (rbtree_t* s, node_t* x)
{
    STF(x, c, RED);
    while (x != NULL && x != LDNODE(s, root)) {
        node_t* xp = LDNODE(x, p);
        if (LDF(xp, c) != RED) {
            break;
        }
        /* TODO: cache g = ppx = PARENT_OF(PARENT_OF(x)) */
        if (PARENT_OF(x) == LEFT_OF(PARENT_OF(PARENT_OF(x)))) {
            node_t*  y = RIGHT_OF(PARENT_OF(PARENT_OF(x)));
            if (COLOR_OF(y) == RED) {
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(y, BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                x = PARENT_OF(PARENT_OF(x));
            } else {
                if (x == RIGHT_OF(PARENT_OF(x))) {
                    x = PARENT_OF(x);
                    ROTATE_LEFT(s, x);
                }
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                if (PARENT_OF(PARENT_OF(x)) != NULL) {
                    ROTATE_RIGHT(s, PARENT_OF(PARENT_OF(x)));
                }
            }
        } else {
            node_t* y = LEFT_OF(PARENT_OF(PARENT_OF(x)));
            if (COLOR_OF(y) == RED) {
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(y, BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                x = PARENT_OF(PARENT_OF(x));
            } else {
                if (x == LEFT_OF(PARENT_OF(x))) {
                    x = PARENT_OF(x);
                    ROTATE_RIGHT(s, x);
                }
                SET_COLOR(PARENT_OF(x),  BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                if (PARENT_OF(PARENT_OF(x)) != NULL) {
                    ROTATE_LEFT(s, PARENT_OF(PARENT_OF(x)));
                }
            }
        }
    }
    node_t* ro = LDNODE(s, root);
    if (LDF(ro, c) != BLACK) {
        STF(ro, c, BLACK);
    }
}
#define FIX_AFTER_INSERTION(s, x)  fixAfterInsertion(s, x)


/* =============================================================================
 * insert
 * =============================================================================
 */
static node_t*
insert (rbtree_t* s, int k, int v, node_t* n)
{
    node_t* t  = LDNODE(s, root);
    if (t == NULL) {
        if (n == NULL) {
            return NULL;
        }
        /* Note: the following STs don't really need to be transactional */
        STF(n, l, NULL);
        STF(n, r, NULL);
        STF(n, p, NULL);
        STF(n, k, k);
        STF(n, v, v);
        STF(n, c, BLACK);
        STF(s, root, n);
        return NULL;
    }

    for (;;) {
        intptr_t cmp = k - LDF(t, k);
        if (cmp == 0) {
            return t;
        } else if (cmp < 0) {
            node_t* tl = LDNODE(t, l);
            if (tl != NULL) {
                t = tl;
            } else {
                STF(n, l, NULL);
                STF(n, r, NULL);
                STF(n, k, k);
                STF(n, v, v);
                STF(n, p, t);
                STF(t, l, n);
                FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        } else { /* cmp > 0 */
            node_t* tr = LDNODE(t, r);
            if (tr != NULL) {
                t = tr;
            } else {
                STF(n, l, NULL);
                STF(n, r, NULL);
                STF(n, k, k);
                STF(n, v, v);
                STF(n, p, t);
                STF(t, r, n);
                FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        }
    }
}
#define INSERT(s, k, v, n)  insert(s, k, v, n)

/*
 * Return the given node's successor node---the node which has the
 * next key in the the left to right ordering. If the node has
 * no successor, a null pointer is returned rather than a pointer to
 * the nil node
 */


/* =============================================================================
 * successor
 * =============================================================================
 */
static node_t*
successor (node_t* t)
{
    if (t == NULL) {
        return NULL;
    } else if (LDNODE(t, r) != NULL) {
        node_t* p = LDNODE(t, r);
        while (LDNODE(p, l) != NULL) {
            p = LDNODE(p, l);
        }
        return p;
    } else {
        node_t* p = LDNODE(t, p);
        node_t* ch = t;
        while (p != NULL && ch == LDNODE(p, r)) {
            ch = p;
            p = LDNODE(p, p);
        }
        return p;
    }
}
#define SUCCESSOR(n)  successor(n)

/* =============================================================================
 * fixAfterDeletion
 * =============================================================================
 */
static void
fixAfterDeletion (rbtree_t* s, node_t*  x)
{
    while (x != LDNODE(s,root) && COLOR_OF(x) == BLACK) {
        if (x == LEFT_OF(PARENT_OF(x))) {
            node_t* sib = RIGHT_OF(PARENT_OF(x));
            if (COLOR_OF(sib) == RED) {
                SET_COLOR(sib, BLACK);
                SET_COLOR(PARENT_OF(x), RED);
                ROTATE_LEFT(s, PARENT_OF(x));
                sib = RIGHT_OF(PARENT_OF(x));
            }

            if (COLOR_OF(LEFT_OF(sib))  == BLACK &&
                COLOR_OF(RIGHT_OF(sib)) == BLACK) {
                SET_COLOR(sib, RED);
                x = PARENT_OF(x);
            } else {
                if (COLOR_OF(RIGHT_OF(sib)) == BLACK) {
                    SET_COLOR(LEFT_OF(sib), BLACK);
                    SET_COLOR(sib, RED);
                    ROTATE_RIGHT(s, sib);
                    sib = RIGHT_OF(PARENT_OF(x));
                }
                SET_COLOR(sib, COLOR_OF(PARENT_OF(x)));
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(RIGHT_OF(sib), BLACK);
                ROTATE_LEFT(s, PARENT_OF(x));
                /* TODO: consider break ... */
                x = LDNODE(s,root);
            }
        } else { /* symmetric */
            node_t* sib = LEFT_OF(PARENT_OF(x));

            if (COLOR_OF(sib) == RED) {
                SET_COLOR(sib, BLACK);
                SET_COLOR(PARENT_OF(x), RED);
                ROTATE_RIGHT(s, PARENT_OF(x));
                sib = LEFT_OF(PARENT_OF(x));
            }

            if (COLOR_OF(RIGHT_OF(sib)) == BLACK &&
                COLOR_OF(LEFT_OF(sib)) == BLACK) {
                SET_COLOR(sib,  RED);
                x = PARENT_OF(x);
            } else {
                if (COLOR_OF(LEFT_OF(sib)) == BLACK) {
                    SET_COLOR(RIGHT_OF(sib), BLACK);
                    SET_COLOR(sib, RED);
                    ROTATE_LEFT(s, sib);
                    sib = LEFT_OF(PARENT_OF(x));
                }
                SET_COLOR(sib, COLOR_OF(PARENT_OF(x)));
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(LEFT_OF(sib), BLACK);
                ROTATE_RIGHT(s, PARENT_OF(x));
                /* TODO: consider break ... */
                x = LDNODE(s, root);
            }
        }
    }

    if (x != NULL && LDF(x,c) != BLACK) {
       STF(x, c, BLACK);
    }
}
#define FIX_AFTER_DELETION(s, n)  fixAfterDeletion(s, n)

/* =============================================================================
 * delete_node
 * =============================================================================
 */
static node_t*
delete_node (rbtree_t* s, node_t* p)
{
    /*
     * If strictly internal, copy successor's element to p and then make p
     * point to successor
     */
    if (LDNODE(p, l) != NULL && LDNODE(p, r) != NULL) {
        node_t* s = SUCCESSOR(p);
        STF(p, k, (intptr_t)LDNODE(s, k));
        STF(p, v, (intptr_t)LDNODE(s, v));
        p = s;
    } /* p has 2 children */

    /* Start fixup at replacement node, if it exists */
    node_t* replacement =
        ((LDNODE(p, l) != NULL) ? LDNODE(p, l) : LDNODE(p, r));

    if (replacement != NULL) {
        /* Link replacement to parent */
        /* TODO: precompute pp = p->p and substitute below ... */
        STF (replacement, p, LDNODE(p, p));
        node_t* pp = LDNODE(p, p);
        if (pp == NULL) {
            STF(s, root, replacement);
        } else if (p == LDNODE(pp, l)) {
            STF(pp, l, replacement);
        } else {
            STF(pp, r, replacement);
        }

        /* Null out links so they are OK to use by fixAfterDeletion */
        STF(p, l, NULL);
        STF(p, r, NULL);
        STF(p, p, NULL);

        /* Fix replacement */
        if (LDF(p,c) == BLACK) {
            FIX_AFTER_DELETION(s, replacement);
        }
    } else if (LDNODE(p, p) == NULL) { /* return if we are the only node */
        STF(s, root, NULL);
    } else { /* No children. Use self as phantom replacement and unlink */
        if (LDF(p, c) == BLACK) {
            FIX_AFTER_DELETION(s, p);
        }
        node_t* pp = LDNODE(p, p);
        if (pp != NULL) {
            if (p == LDNODE(pp, l)) {
                STF(pp,l, NULL);
            } else if (p == LDNODE(pp, r)) {
                STF(pp, r, NULL);
            }
            STF(p, p, NULL);
        }
    }
    return p;
}
#define delete_node(s, n)  delete_node(s, n)


/*
 * Diagnostic section
 */


/* =============================================================================
 * firstEntry
 * =============================================================================
 */
static node_t*
firstEntry (rbtree_t* s)
{
    node_t* p = s->root;
    if (p != NULL) {
        while (p->l != NULL) {
            p = p->l;
        }
    }
    return p;
}


/*
 * Compute the BH (BlackHeight) and validate the tree.
 *
 * This function recursively verifies that the given binary subtree satisfies
 * three of the red black properties. It checks that every red node has only
 * black children. It makes sure that each node is either red or black. And it
 * checks that every path has the same count of black nodes from root to leaf.
 * It returns the blackheight of the given subtree; this allows blackheights to
 * be computed recursively and compared for left and right siblings for
 * mismatches. It does not check for every nil node being black, because there
 * is only one sentinel nil node. The return value of this function is the
 * black height of the subtree rooted at the node ``root'', or zero if the
 * subtree is not red-black.
 *
 */


/* =============================================================================
 * verifyRedBlack
 * =============================================================================
 */
static int
verifyRedBlack (node_t* root, int depth)
{
    int height_left;
    int height_right;

    if (root == NULL) {
        return 1;
    }

    height_left  = verifyRedBlack(root->l, depth+1);
    height_right = verifyRedBlack(root->r, depth+1);
    if (height_left == 0 || height_right == 0) {
        return 0;
    }
    if (height_left != height_right) {
        printf(" Imbalance @depth=%d : %d %d\n", depth, height_left, height_right);
    }

    if (root->l != NULL && root->l->p != root) {
       printf(" lineage\n");
    }
    if (root->r != NULL && root->r->p != root) {
       printf(" lineage\n");
    }

    /* Red-Black alternation */
    if (root->c == RED) {
        if (root->l != NULL && root->l->c != BLACK) {
          printf("VERIFY %d\n", __LINE__);
          return 0;
        }
        if (root->r != NULL && root->r->c != BLACK) {
          printf("VERIFY %d\n", __LINE__);
          return 0;
        }
        return height_left;
    }
    if (root->c != BLACK) {
        printf("VERIFY %d\n", __LINE__);
        return 0;
    }
    return (height_left + 1);
}


/* =============================================================================
 * rbtree_verify
 * =============================================================================
 */
int
rbtree_verify (rbtree_t* s, int verbose)
{
    node_t* root = s->root;
    if (root == NULL) {
        return 1;
    }
    if (verbose) {
       printf("Integrity check: ");
    }

    if (root->p != NULL) {
        printf("  (WARNING) root %lX parent=%lX\n",
               (unsigned long)root, (unsigned long)root->p);
        return -1;
    }
    if (root->c != BLACK) {
        printf("  (WARNING) root %lX color=%lX\n",
               (unsigned long)root, (unsigned long)root->c);
    }

    /* Weak check of binary-tree property */
    int ctr = 0;
    node_t* its = firstEntry(s);
    while (its != NULL) {
        ctr++;
        node_t* child = its->l;
        if (child != NULL && child->p != its) {
            printf("Bad parent\n");
        }
        child = its->r;
        if (child != NULL && child->p != its) {
            printf("Bad parent\n");
        }
        node_t* nxt = successor(its);
        if (nxt == NULL) {
            break;
        }
        if (its->k >= nxt->k) {
            printf("Key order %lX (%ld %ld) %lX (%ld %ld)\n",
                   (unsigned long)its, (long)its->k, (long)its->v,
                   (unsigned long)nxt, (long)nxt->k, (long)nxt->v);
            return -3;
        }
        its = nxt;
    }

    int vfy = verifyRedBlack(root, 0);
    if (verbose) {
        printf(" Nodes=%d Depth=%d\n", ctr, vfy);
    }
    return vfy;
}


/* =============================================================================
 * rbtree_alloc
 * =============================================================================
 */
rbtree_t*
rbtree_alloc ()
{
    rbtree_t* n = (rbtree_t* )malloc(sizeof(*n));
    n->root = NULL;
    return n;
}

/* =============================================================================
 * rbtree_free
 * =============================================================================
 */
void
rbtree_free (rbtree_t* r)
{
    free(r);
}


/* =============================================================================
 * getNode
 * =============================================================================
 */
static node_t*
getNode ()
{
    node_t* n = (node_t*)malloc(sizeof(*n));
    return n;
}

/* =============================================================================
 * releaseNode
 * =============================================================================
 */
static void
releaseNode (node_t* n)
{
    free(n);
}

/* =============================================================================
 * rbtree_insert
 * =============================================================================
 */
__attribute__((transaction_safe)) int
rbtree_insert (rbtree_t* r, int key, int val)
{
    node_t* node = getNode();
    node_t* ex = INSERT(r, key, val, node);
    if (ex != NULL) {
        releaseNode(node);
    }
    return (ex != NULL);
}

/* =============================================================================
 * rbtree_delete
 * =============================================================================
 */
__attribute__((transaction_safe)) int
rbtree_delete (rbtree_t* r, int key)
{
    node_t* node = NULL;
    node = LOOKUP(r, key);
    if (node != NULL) {
        node = delete_node(r, node);
    }
    if (node != NULL) {
        releaseNode(node);
    }
    return (node != NULL);
}

/* =============================================================================
 * rbtree_update
 * =============================================================================
 */
int
rbtree_update (rbtree_t* r, int key, int val)
{
    node_t* nn = getNode();
    node_t* ex = INSERT(r, key, val, nn);
    if (ex != NULL) {
        STF(ex, v, val);
        releaseNode(nn);
        return 0;
    }
    return 1;
}

/* =============================================================================
 * rbtree_get
 * =============================================================================
 */
int
rbtree_get (rbtree_t* r, int key) {
    node_t* n = LOOKUP(r, key);
    if (n != NULL) {
        int val = LDF(n, v);
        return val;
    }
    return 0;
}

/* =============================================================================
 * rbtree_contains
 * =============================================================================
 */
__attribute__((transaction_safe)) int
rbtree_contains (rbtree_t* r, int key)
{
    node_t* n = LOOKUP(r, key);
    return n != NULL;
}
