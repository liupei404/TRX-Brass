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

#pragma once

#include <stddef.h>

#include "src/settings/bands.h"
#include "list.h"

typedef struct band_tree     band_tree_t;
typedef struct band_treetrav band_treetrav_t;

band_tree_t *band_tree_new();
void band_tree_delete(band_tree_t *tree);
band_t *band_tree_find(band_tree_t *tree, uint64_t start, uint64_t stop);
band_list_t *band_tree_findall(band_tree_t *tree, uint64_t start, uint64_t stop);
int band_tree_insert(band_tree_t *tree, band_t *interval);
size_t band_tree_size(band_tree_t *tree);

band_treetrav_t *band_treetnew(void);
void band_treetdelete(band_treetrav_t *trav);
band_t *band_treetfirst(band_treetrav_t *trav, band_tree_t *tree);
band_t *band_treetlast(band_treetrav_t *trav, band_tree_t *tree);
band_t *band_treetnext(band_treetrav_t *trav);
band_t *band_treetprev(band_treetrav_t *trav);
