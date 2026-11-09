/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <liquid/liquid.h>

#include "generator.h"

complex float generator_noise() {
    return randnf() + _Complex_I * randnf();
}

void generator_tone_set_freq(generator_tone_t *tone, float freq, float rate) {
    tone->delta = 2.0f * M_PI * freq / rate;
}

complex float generator_tone(generator_tone_t *tone) {
    tone->phase += tone->delta;

    return cexpf(_Complex_I * tone->phase);
}
