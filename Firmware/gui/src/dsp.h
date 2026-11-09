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
#include "settings/modes.h"

void dsp_init();
void dsp_reset();

void dsp_set_spectrum_factor(uint8_t x);
void dsp_set_filter(filter_t *filter);
void dsp_set_rx_agc(uint8_t mode);

float dsp_get_spectrum_beta();
void dsp_set_spectrum_beta(float x);

void dsp_auto_clear();

void dsp_change_mute();
void dsp_set_mute(bool on);
void dsp_set_vol(uint8_t x);
uint16_t dsp_change_vol(int16_t df);
uint8_t dsp_change_rx_agc(int16_t df);

int dsp_change_denoise(int16_t d);
void dsp_update_denoise();

float complex dsp_modulate(float x, radio_mode_t mode);
float dsp_demodulate(float complex in, radio_mode_t mode);

void dsp_update_equalizer();

/* From thread */

void dsp_fft(float *data);
void dsp_adc(float complex *data, uint16_t samples);
size_t dsp_dac(float complex *data, size_t max_size);
