/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cyaml/cyaml.h>

typedef struct {
    char                *label;
    uint16_t            x[7];
    uint16_t            y[5];
} hmic_item_t;

typedef struct {
    hmic_item_t         *hmic;
    uint16_t            hmic_count;
    uint32_t            tcxo_freq;
} hw_t;

extern hw_t   *hw;

void settings_hw_load();
void settings_hw_save();
