/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "op_work.h"

typedef struct {
    const char      *label;
    uint64_t        start;
    uint64_t        stop;
    bool            tx;
    bool            jump;
} band_settings_t;

typedef struct {
    band_settings_t settings;
    op_work_t       work;
} band_t;

typedef struct {
    op_work_t   initial;
    band_t      *item;
    unsigned    count;
} bands_t;

extern bands_t *bands;

void settings_bands_load();
void settings_bands_save();
