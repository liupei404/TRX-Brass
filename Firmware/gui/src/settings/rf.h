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

#include "rx_band.h"
#include "tx_band.h"
#include "xvrt.h"

typedef enum {
    RF_NORMAL = 0,
    RF_TWO_TONE,
    RF_SILENCE
} rf_mode_t;

typedef struct {
    int     adc;
    float   vpp;
} rf_calibrate_t;

typedef struct {
    int             fwd_offset;
    int             rev_offset;
    rf_calibrate_t  *calibrate;
    unsigned        count;
} rf_swr_t;

typedef struct {
    rf_mode_t       mode;
    uint16_t        pa_bias[2];
    uint16_t        two_tone[2];
    rx_band_item_t  *rx_band;
    uint16_t        rx_band_count;
    tx_band_item_t  *tx_band;
    uint16_t        tx_band_count;
    xvrt_item_t     *xvrt;
    uint64_t        xvrt_count;
    int32_t         txo_offset;
    float           ref_pwr;
    float           pwr;
    uint8_t         ant;
    bool            atu;
    bool            atu_loaded;
    rf_swr_t        swr;
} rf_t;

extern rf_t   *rf;

void settings_rf_load();
void settings_rf_save();
