/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 *
 *  Based on Libitree: an interval tree library in C
 *  Copyright (C) 2018 Alessandro Vullo
 */

#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "tree.h"
#include "interval.h"

#ifndef HEIGHT_LIMIT
#    define HEIGHT_LIMIT 64 /* Tallest allowable tree */
#endif

typedef struct band_treenode {
    int                     balance;
    uint64_t                max;
    band_t                  *band;
    struct band_treenode    *link[2];
} band_treenode_t;

struct band_tree {
    band_treenode_t     *root;
    size_t              size;
};

struct band_treetrav {
    band_tree_t     *tree;
    band_treenode_t *it;
    band_treenode_t *path[HEIGHT_LIMIT];
    size_t          top;
};

/* Two way single rotation */
#define single(root, dir)                                                                                              \
    do {                                                                                                               \
        band_treenode_t   *save = root->link[!dir];                                                                    \
        root->link[!dir]  = save->link[dir];                                                                           \
        save->link[dir]   = root;                                                                                      \
        root              = save;                                                                                      \
    } while (0)

/* Two way double rotation */
#define double(root, dir)                                                                                              \
    do {                                                                                                               \
        band_treenode_t *save       = root->link[!dir]->link[dir];                                                     \
        root->link[!dir]->link[dir] = save->link[!dir];                                                                \
        save->link[!dir]            = root->link[!dir];                                                                \
        root->link[!dir]            = save;                                                                            \
        save                        = root->link[!dir];                                                                \
        root->link[!dir]            = save->link[dir];                                                                 \
        save->link[dir]             = root;                                                                            \
        root                        = save;                                                                            \
    } while (0)

/* Adjust balance before double rotation */
#define adjust_balance(root, dir, bal)                                                                                 \
    do {                                                                                                               \
        band_treenode_t *n  = root->link[dir];                                                                         \
        band_treenode_t *nn = n->link[!dir];                                                                           \
        if (nn->balance == 0)                                                                                          \
            root->balance = n->balance = 0;                                                                            \
        else if (nn->balance == bal) {                                                                                 \
            root->balance = -bal;                                                                                      \
            n->balance    = 0;                                                                                         \
        } else { /* nn->balance == -bal */                                                                             \
            root->balance = 0;                                                                                         \
            n->balance    = bal;                                                                                       \
        }                                                                                                              \
        nn->balance = 0;                                                                                               \
    } while (0)

/* Rebalance after insertion */
#define insert_balance(root, dir)                                                                                      \
    do {                                                                                                               \
        band_treenode_t *n   = root->link[dir];                                                                        \
        int              bal = dir == 0 ? -1 : +1;                                                                     \
        if (n->balance == bal) {                                                                                       \
            root->balance = n->balance = 0;                                                                            \
            single(root, !dir);                                                                                        \
        } else { /* n->balance == -bal */                                                                              \
            adjust_balance(root, dir, bal);                                                                            \
            double(root, !dir);                                                                                        \
        }                                                                                                              \
    } while (0)

/* Rebalance after deletion */
#define remove_balance(root, dir, done)                                                                                \
    do {                                                                                                               \
        band_treenode_t *n   = root->link[!dir];                                                                       \
        int              bal = dir == 0 ? -1 : +1;                                                                     \
        if (n->balance == -bal) {                                                                                      \
            root->balance = n->balance = 0;                                                                            \
            single(root, dir);                                                                                         \
        } else if (n->balance == bal) {                                                                                \
            adjust_balance(root, !dir, -bal);                                                                          \
            double(root, dir);                                                                                         \
        } else { /* n->balance == 0 */                                                                                 \
            root->balance = -bal;                                                                                      \
            n->balance    = bal;                                                                                       \
            single(root, dir);                                                                                         \
            done = 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static band_treenode_t *new_node(band_tree_t *tree, band_t *i) {
    band_treenode_t *rn = (band_treenode_t *) malloc(sizeof *rn);

    if (rn == NULL) {
        return NULL;
    }

    rn->band = i;

    rn->balance = 0;
    rn->max     = i->settings.stop;
    rn->link[0] = rn->link[1] = NULL;

    return rn;
}

band_tree_t *band_tree_new() {
    band_tree_t *rt = (band_tree_t *) malloc(sizeof *rt);

    if (rt == NULL) {
        return NULL;
    }

    rt->root = NULL;
    rt->size = 0;

    return rt;
}

void band_tree_delete(band_tree_t *tree) {
    band_treenode_t *it = tree->root;
    band_treenode_t *save;

    /* Destruction by rotation */
    while (it != NULL) {
        if (it->link[0] == NULL) {
            /* Remove node */
            save = it->link[1];
            free(it);
        } else {
            /* Rotate right */
            save          = it->link[0];
            it->link[0]   = save->link[1];
            save->link[1] = it;
        }

        it = save;
    }

    free(tree);
}

static band_t *find(band_treenode_t *node, uint64_t start, uint64_t stop) {
    if (node == NULL || start > node->max) {
        return NULL;
    }

    if (node->link[0] != NULL && node->link[0]->max >= start) {
        band_t *res = find(node->link[0], start, stop);

        if (res) {
            return res;
        }
    }

    if (band_overlap(node->band, start, stop)) {
        return node->band;
    }

    if (stop < node->band->settings.start) {
        return NULL;
    }

    return find(node->link[1], start, stop);
}

band_t *band_tree_find(band_tree_t *tree, uint64_t start, uint64_t stop) {
    find(tree->root, start, stop);
}

static void search(band_treenode_t *node, uint64_t start, uint64_t stop, band_list_t *results) {
    if (node == NULL || start > node->max) {
        return;
    }

    if (node->link[0] != NULL && node->link[0]->max >= start) {
        search(node->link[0], start, stop, results);
    }

    if (band_overlap(node->band, start, stop)) {
        band_list_append(results, node->band);
    }

    if (stop < node->band->settings.start) {
        return;
    }

    search(node->link[1], start, stop, results);
}

band_list_t *band_tree_findall(band_tree_t *tree, uint64_t start, uint64_t stop) {
    band_list_t *results = band_list_new();

    if (results != NULL) {
        if (tree->root == NULL) {
            return results;
        }

        search(tree->root, start, stop, results);
    }

    return results;
}

int band_tree_insert(band_tree_t *tree, band_t *interval) {
    if (tree->root == NULL) {
        tree->root = new_node(tree, interval);
        if (tree->root == NULL) {
            return 0;
        }
    } else {
        band_treenode_t head = {0};
        band_treenode_t *s, *t;
        band_treenode_t *p, *q;
        int             dir;

        t          = &head;
        t->link[1] = tree->root;

        for (s = p = t->link[1];; p = q) {
            dir    = p->band->settings.start <= interval->settings.start;
            q      = p->link[dir];

            p->max = p->max < interval->settings.stop ? interval->settings.stop : p->max; /* Update ancestor's max if needed */

            if (q == NULL) {
                break;
            }

            if (q->balance != 0) {
                t = p;
                s = q;
            }
        }

        p->link[dir] = q = new_node(tree, interval);

        if (q == NULL) {
            return 0;
        }

        /* Update balance factors */
        for (p = s; p != q; p = p->link[dir]) {
            dir = p->band->settings.start <= interval->settings.start;
            p->balance += dir == 0 ? -1 : +1;
        }

        q = s; /* Save rebalance point for parent fix */

        /* Rebalance if necessary */
        if (abs(s->balance) > 1) {
            dir = s->band->settings.start <= interval->settings.start;
            insert_balance(s, dir);
        }

        /* Fix parent */
        if (q == head.link[1]) {
            tree->root = s;
        } else {
            t->link[q == t->link[1]] = s;
        }
    }

    ++tree->size;

    return 1;
}

size_t band_tree_size(band_tree_t *tree) {
    return tree->size;
}

band_treetrav_t *band_treetnew(void) {
    return (band_treetrav_t *) malloc(sizeof(band_treetrav_t));
}

void band_treetdelete(band_treetrav_t *trav) {
    free(trav);
}

static band_t *start(band_treetrav_t *trav, band_tree_t *tree, int dir) {
    trav->tree = tree;
    trav->it   = tree->root;
    trav->top  = 0;

    /* Build a path to work with */
    if (trav->it != NULL) {
        while (trav->it->link[dir] != NULL) {
            trav->path[trav->top++] = trav->it;
            trav->it                = trav->it->link[dir];
        }
    }

    return trav->it == NULL ? NULL : trav->it->band;
}

static band_t *move(band_treetrav_t *trav, int dir) {
    if (trav->it->link[dir] != NULL) {
        /* Continue down this branch */
        trav->path[trav->top++] = trav->it;
        trav->it                = trav->it->link[dir];

        while (trav->it->link[!dir] != NULL) {
            trav->path[trav->top++] = trav->it;
            trav->it                = trav->it->link[!dir];
        }
    } else {
        /* Move to the next branch */
        band_treenode_t *last;

        do {
            if (trav->top == 0) {
                trav->it = NULL;
                break;
            }

            last     = trav->it;
            trav->it = trav->path[--trav->top];
        } while (last == trav->it->link[dir]);
    }

    return trav->it == NULL ? NULL : trav->it->band;
}

band_t *band_treetfirst(band_treetrav_t *trav, band_tree_t *tree) {
    return start(trav, tree, 0);
}

band_t *band_treetlast(band_treetrav_t *trav, band_tree_t *tree) {
    return start(trav, tree, 1);
}

band_t *band_treetnext(band_treetrav_t *trav) {
    return move(trav, 1);
}

band_t *band_treetprev(band_treetrav_t *trav) {
    return move(trav, 0);
}
