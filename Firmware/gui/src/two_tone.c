/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "two_tone.h"
#include "generator.h"
#include "settings/rf.h"
#include "fpga/dac.h"

static generator_tone_t         tone[2];

void two_tone_update() {
    generator_tone_set_freq(&tone[0], rf->two_tone[0], DAC_RATE);
    generator_tone_set_freq(&tone[1], rf->two_tone[1], DAC_RATE);
}

size_t two_tone_generator(float complex *data, size_t max_size) {
    size_t size = 128;

    for (uint16_t i = 0; i < size; i++) {
        data[i] = generator_tone(&tone[0]) * 0.5f + generator_tone(&tone[1]) * 0.5f;
    }

    return size;
}
