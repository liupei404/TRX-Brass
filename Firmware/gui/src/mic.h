/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <complex.h>
#include <stddef.h>
#include <stdint.h>

#include "radio.h"

void mic_init();
size_t mic_modulate(float complex *data, size_t max_size, radio_mode_t mode);
void mic_on_air(bool on);
void mic_enabled(bool on);

void mic_update_filter();
void mic_update_equalizer();

void mic_put_audio_samples(size_t nsamples, int16_t *samples);
