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

typedef enum {
    VOL_VOL = 0,
    VOL_PWR,
    VOL_FREQ_MODE,
    VOL_SPLIT,
    VOL_VOICE_LANG,
    VOL_VOICE_RATE,
    VOL_VOICE_PITCH,
    VOL_VOICE_VOLUME,
    
    VOL_LAST
} vol_mode_t;

void vol_update(int16_t diff, bool voice);
void vol_press(int16_t dir);
void vol_set_mode(vol_mode_t mode);
