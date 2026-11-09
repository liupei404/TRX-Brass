/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#include <cstdarg>
#endif

void olivia_init();
int olivia_put_audio_samples(int16_t *buf, size_t samples);

void olivia_info_cb(lv_event_t * e);
void olivia_preset(uint16_t x);

uint16_t olivia_change_tones(int16_t df);
uint16_t olivia_change_width(int16_t df);

#ifdef __cplusplus
}
#endif
