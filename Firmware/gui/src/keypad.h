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

    lv_indev_t      *indev;

    int             evdev_state;
    int             evdev_key;
} keypad_t;

keypad_t * keypad_init(char *dev_name);
