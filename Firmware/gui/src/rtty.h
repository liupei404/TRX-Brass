/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <liquid/liquid.h>

typedef enum {
    RTTY_OFF = 0,
    RTTY_RX,
    RTTY_TX
} rtty_state_t;

void rtty_init();

void rtty_set_state(rtty_state_t state);
rtty_state_t rtty_get_state();

float rtty_change_rate(int16_t df);
uint16_t rtty_change_shift(int16_t df);
uint16_t rtty_change_center(int16_t df);
bool rtty_change_reverse(int16_t df);

/* From thread */

void rtty_put_audio_samples(float complex *samples, size_t n);
