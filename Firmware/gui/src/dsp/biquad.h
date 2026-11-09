/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

typedef struct {
    float   a1, a2;
    float   b0, b1, b2;
    float   x1, x2;
    float   y1, y2;
} biquad_t;

float biqiad_apply(biquad_t *f, float x);

void biquad_lpf(biquad_t *f, float f0, float q, float fs);
void biquad_hpf(biquad_t *f, float f0, float q, float fs);
void biquad_peak_eq(biquad_t *f, float f0, float q, float gain, float fs);
