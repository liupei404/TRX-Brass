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
#include "src/ft8/constants.h"

typedef enum {
    FT8_SHOW_ALL = 0,
    FT8_SHOW_CQ
} ft8_show_t;

typedef struct {
    ftx_protocol_t  protocol;
    uint8_t         ft8_band;
    uint8_t         ft4_band;
    ft8_show_t      show;
    bool            tx_auto;
    uint16_t        tx_freq;
} settings_ft8_t;

extern settings_ft8_t   *settings_ft8;

void settings_ft8_load();
void settings_ft8_save();
