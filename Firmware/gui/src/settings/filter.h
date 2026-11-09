/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <cyaml/cyaml.h>

typedef struct {
    uint16_t    low;
    uint16_t    high;
    uint16_t    transition;
} filter_t;

extern const cyaml_schema_field_t filter_fields_schema[];
