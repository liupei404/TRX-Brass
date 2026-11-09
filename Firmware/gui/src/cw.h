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

void cw_init();

bool cw_change_decoder(int16_t df);
float cw_change_snr(int16_t df);
float cw_change_peak_beta(int16_t df);
float cw_change_noise_beta(int16_t df);

/* From thread */

void cw_put_audio_samples(float complex *samples, size_t n);
