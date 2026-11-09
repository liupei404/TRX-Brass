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

#include "list.h"

#ifndef HEIGHT_LIMIT
#    define HEIGHT_LIMIT 64
#endif

struct band_listnode {
    const band_t            *band;
    struct band_listnode    *next;
};

struct band_list {
    band_listnode_t *head, *tail;
    size_t          size;
};

struct band_listtrav {
    band_list_t     *list;
    band_listnode_t *it;
};

static band_listnode_t *band_listnode_new(const band_t *band, band_listnode_t *next) {
    band_listnode_t *node = (band_listnode_t *) malloc(sizeof *node);

    if (node != NULL) {
        node->band = band;
        node->next = next;
    }

    return node;
}

band_listnode_t *band_listnode_delete(band_listnode_t *node) {
    band_listnode_t *next = NULL;

    if (node != NULL) {
        next = node->next;

        free(node);
    }

    return next;
}

band_list_t *band_list_new() {
    band_list_t *list = (band_list_t *) malloc(sizeof *list);

    if (list != NULL) {
        band_listnode_t *tail = band_listnode_new(NULL, NULL);

        if (tail == NULL) {
            free(list);
            list = NULL;
        } else {
            list->tail = tail;
            list->head = band_listnode_new(NULL, list->tail);

            if (list->head == NULL) {
                free(list);
                list = NULL;
            }

            list->size = 0;
        }
    }

    return list;
}

void band_list_delete(band_list_t *list) {
    if (list == NULL) {
        return;
    }

    band_listnode_t *it = list->head;

    while (it != list->tail) {
        it = band_listnode_delete(it);
    }

    band_listnode_delete(it);
    free(list);
}

size_t band_list_size(band_list_t *list) {
    return list->size;
}

static band_listnode_t *insert_before(band_list_t *, band_listnode_t *, band_t *);

static band_listnode_t *insert_after(band_list_t *list, band_listnode_t *pos, band_t *i) {
    band_listnode_t *node = NULL;

    if (list != NULL && pos != NULL) {
        if (pos != list->tail) {
            node = band_listnode_new(i, pos->next);

            if (node != NULL) {
                pos->next = node;
                ++list->size;
            }
        } else {
            node = insert_before(list, pos, i);
        }
    }

    return node;
}

static band_listnode_t *insert_before(band_list_t *list, band_listnode_t *pos, band_t *i) {
    band_listnode_t *node = NULL;

    if (list != NULL && pos != NULL) {
        if (pos != list->head) {
            band_listnode_t *it = list->head;

            while (it->next != pos) {
                it = it->next;
            }

            node = band_listnode_new(i, it->next);

            if (node != NULL) {
                it->next = node;
                ++list->size;
            }
        } else {
            node = insert_after(list, pos, i);
        }
    }

    return node;
}

int band_list_append(band_list_t *list, band_t *i) {
    band_listnode_t *node = insert_before(list, list->tail, i);

    if (node != NULL) {
        return 1;
    }

    return 0;
}

band_listtrav_t *band_listtrav_new(band_list_t *list) {
    if (list == NULL) {
        return NULL;
    }

    band_listtrav_t *trav = (band_listtrav_t *) malloc(sizeof(band_listtrav_t));

    if (trav != NULL) {
        trav->list = list;
    }

    return trav;
}

void band_listtrav_delete(band_listtrav_t *trav) {
    free(trav);
}

const band_t *band_listtrav_first(band_listtrav_t *trav) {
    if (trav->list == NULL) {
        return NULL;
    }

    trav->it = trav->list->head->next;

    return trav->it == NULL || trav->it == trav->list->tail ? NULL : trav->it->band;
}

const band_t *band_listtrav_last(band_listtrav_t *trav) {
    if (trav->list == NULL) {
        return NULL;
    }

    trav->it = trav->list->head;

    while (trav->it->next != trav->list->tail) {
        trav->it = trav->it->next;
    }

    return trav->it == trav->list->head || trav->it == NULL ? NULL : trav->it->band;
}

const band_t *band_listtrav_next(band_listtrav_t *trav) {
    if (trav == NULL) {
        return NULL;
    }

    trav->it = trav->it->next;

    return trav->it == NULL || trav->it == trav->list->tail ? NULL : trav->it->band;
}

const band_t *band_listtrav_prev(band_listtrav_t *trav) {
    if (trav == NULL) {
        return NULL;
    }

    band_listnode_t *it = trav->list->head;

    while (it->next != trav->it) {
        it = it->next;
    }

    trav->it = it;

    return trav->it == trav->list->head || trav->it == NULL ? NULL : trav->it->band;
}
