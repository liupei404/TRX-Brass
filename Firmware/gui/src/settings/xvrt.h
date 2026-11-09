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
    uint64_t        start;
    uint64_t        stop;
    uint32_t        shift;
    int32_t         corr;
} xvrt_item_t;

extern const cyaml_schema_value_t xvrt_schema;
