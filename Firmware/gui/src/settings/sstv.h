/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint8_t partial_min_percent;
} settings_sstv_t;

extern settings_sstv_t *settings_sstv;

void settings_sstv_load(void);
void settings_sstv_save(void);
