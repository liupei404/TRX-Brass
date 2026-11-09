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
#include <stddef.h>
#include "settings/options.h"

void cw_key_init();

uint16_t cw_key_change_speed(int16_t d);
key_mode_t cw_key_change_mode(int16_t d);
iambic_mode_t cw_key_change_iambic_mode(int16_t d);
uint16_t cw_key_change_tone(int16_t d);
bool cw_key_change_train(int16_t d);
uint16_t cw_key_change_qsk_time(int16_t d);
uint8_t cw_key_change_ratio(int16_t d);

#ifndef __cplusplus
#include <complex.h>

size_t cw_key_generator(float complex *data, size_t max_size, bool reverse);

#endif
