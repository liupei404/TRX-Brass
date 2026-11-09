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

typedef struct {
    bool                linear;
    uint32_t            span;
} settings_swrscan_t;

extern settings_swrscan_t   *settings_swrscan;

void settings_swrscan_load(); // DOLATER
void settings_swrscan_save(); // DOLATER
