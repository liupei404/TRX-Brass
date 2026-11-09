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
    uint32_t        to;
    uint16_t        lpf;
    uint16_t        vref;
} tx_band_item_t;

extern const cyaml_schema_value_t tx_band_schema;
