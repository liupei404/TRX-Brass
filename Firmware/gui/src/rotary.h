/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include "lvgl/lvgl.h"
#include "events.h"

typedef struct {
    int             fd;
    uint16_t        left[3];
    uint16_t        right[3];
    uint8_t         mode;
    uint8_t         div;
    
    lv_indev_t      *indev;
    int16_t         accum;
} rotary_t;

rotary_t * rotary_init(char *dev_name, uint8_t div);
