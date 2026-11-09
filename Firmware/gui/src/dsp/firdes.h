/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <unistd.h>

void firdes_band_pass(float gain, float sample_freq, float low_freq, float high_freq, float *taps, size_t taps_len);
size_t firdes_compute_taps_len(float sample_rate, float width, float attenuation);
