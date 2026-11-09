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

typedef struct band_listnode    band_listnode_t;
typedef struct band_list        band_list_t;
typedef struct band_listtrav    band_listtrav_t;

band_list_t *band_list_new();
void band_list_delete(band_list_t *);
size_t band_list_size(band_list_t *);
int band_list_append(band_list_t *, band_t *);

band_listtrav_t *band_listtrav_new(band_list_t *);
void band_listtrav_delete(band_listtrav_t *trav);
const band_t *band_listtrav_first(band_listtrav_t *trav);
const band_t *band_listtrav_last(band_listtrav_t *trav);
const band_t *band_listtrav_next(band_listtrav_t *trav);
const band_t *band_listtrav_prev(band_listtrav_t *trav);
