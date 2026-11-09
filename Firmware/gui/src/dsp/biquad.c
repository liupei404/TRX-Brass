/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <math.h>
#include "biquad.h"

float biqiad_apply(biquad_t *f, float x) {
    float y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2 - f->a1 * f->y1 - f->a2 * f->y2;
    
    f->x2 = f->x1;
    f->x1 = x;
    
    f->y2 = f->y1;
    f->y1 = y;
    
    return y;
}

void biquad_lpf(biquad_t *f, float f0, float q, float fs) {
    float w0 = 2.0f * M_PI * f0 / fs;
    float alpha = sinf(w0) / (2.0f * q);
    float cosw0 = cosf(w0);
    float a0 = 1.0f + alpha;

    f->b0 = ((1.0f - cosw0) / 2.0f ) / a0;
    f->b1 = (1.0f - cosw0) / a0;
    f->b2 = f->b0;
    f->a1 = (-2.0f * cosw0) / a0;
    f->a2 = (1.0f - alpha) / a0;
}

void biquad_hpf(biquad_t *f, float f0, float q, float fs) {
    float w0 = 2.0f * M_PI * f0 / fs;
    float alpha = sinf(w0) / (2.0f * q);
    float cosw0 = cosf(w0);
    float a0 = 1.0f + alpha;

    f->b0 = ((1.0f + cosw0) / 2.0f ) / a0;
    f->b1 = -(1.0f + cosw0) / a0;
    f->b2 = f->b0;
    f->a1 = (-2.0f * cosw0) / a0;
    f->a2 = (1.0f - alpha) / a0;
}

void biquad_peak_eq(biquad_t *f, float f0, float q, float gain, float fs) {
    float w0 = 2.0f * M_PI * f0 / fs;
    float a = powf(10.0f, gain/40.0f);
    float alpha = sinf(w0) / (2.0f * q);
    float cosw0 = cosf(w0);
    float a0 = 1.0f + alpha / a;

    f->b0 = (1.0f + alpha * a) / a0;
    f->b1 = (-2.0f * cosw0) / a0;
    f->b2 = (1.0f - alpha * a) / a0;
    f->a1 = (-2.0f * cosw0) / a0;
    f->a2 = (1.0f - alpha/a) / a0;
}
