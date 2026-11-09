/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <complex.h>

typedef struct {
    float delta;
    float phase;
} generator_tone_t;

complex float generator_noise();

void generator_tone_set_freq(generator_tone_t *tone, float freq, float rate);
complex float generator_tone(generator_tone_t *tone);
