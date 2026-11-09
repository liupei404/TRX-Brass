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
#include "filter.h"
#include "src/dsp/agc.h"
#include "src/radio.h"
#include "src/dmr.h"

typedef struct {
    filter_t    filter;
    agc_mode_t  agc;
    uint16_t    freq_step;
    uint16_t    spectrum_factor;
    bool        is_used;
} op_mode_t;

typedef struct {
    op_mode_t       common;
    dmr_config_t    config;
} op_dmr_t;

typedef struct {
    op_mode_t   lsb;
    op_mode_t   usb;
    op_mode_t   cw;
    op_mode_t   cwr;
    op_mode_t   am;
    op_mode_t   nfm;
    op_mode_t   rtty;
    op_mode_t   olivia;
    op_dmr_t    dmr;
} op_modes_t;

extern op_modes_t   *op_modes;
extern op_mode_t    *op_mode;

void settings_modes_load();
void settings_modes_save();

void settings_mode_update(radio_mode_t mode);
