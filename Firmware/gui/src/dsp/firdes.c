/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <liquid/liquid.h>
#include <math.h>
#include "firdes.h"

void firdes_band_pass(float gain, float sample_freq, float low_freq, float high_freq, float *taps, size_t taps_len) {
    float w[taps_len];
    
    for (int i = 0; i < taps_len; i++) {
        w[i] = liquid_windowf(LIQUID_WINDOW_HAMMING, i, taps_len, 0.0f);
    }
    
    int M = (taps_len - 1) / 2;
    float fwT0 = 2.0f * M_PI * low_freq / sample_freq;
    float fwT1 = 2.0f * M_PI * high_freq / sample_freq;

    for (int n = -M; n <= M; n++) {
        if (n == 0) {
            taps[n + M] = (fwT1 - fwT0) / M_PI * w[n + M];
        } else {
            taps[n + M] = (sinf(n * fwT1) - sinf(n * fwT0)) / (n * M_PI) * w[n + M];
        }
    }

    float fmax = taps[M];

    for (int n = 1; n <= M; n++) {
        fmax += 2.0f * taps[n + M] * cosf(n * (fwT0 + fwT1) * 0.5f);
    }

    gain /= fmax;

    for (int i = 0; i < taps_len; i++) {
        taps[i] *= gain;
    }
}

size_t firdes_compute_taps_len(float sample_rate, float width, float attenuation) {
    size_t len = (size_t) (attenuation * sample_rate / (22.0f * width));
    
    len |= 1;   /* make odd */
    
    return len;
}
