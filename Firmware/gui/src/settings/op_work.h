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
#include <cyaml/cyaml.h>

#include "src/radio.h"

typedef struct {
    uint64_t        rx;
    uint64_t        tx;
    uint64_t        fft;
    radio_mode_t    mode;
    split_mode_t    split;
    radio_pre_t     pre;    // DOLATER
    radio_att_t     att;    // DOLATER
    uint32_t        shift;
    int32_t         corr;
} op_work_t;

extern op_work_t                    *op_work;
extern const cyaml_schema_field_t   op_work_fields_schema[];

void op_work_activate(bool set_freq);
void op_work_set_mode(radio_mode_t mode);

void op_work_keep();
void op_work_restore();
