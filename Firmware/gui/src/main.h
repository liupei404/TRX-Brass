/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include "rotary.h"
#include "encoder.h"

#define VERSION "v0.6.0"

typedef enum {
    VOL_EDIT = 0,
    VOL_SELECT,
} vol_rotary_t;

extern rotary_t     *vol;
extern encoder_t    *mfk;

void brass_lv_lock();
void brass_lv_unlock();

void main_exit();
