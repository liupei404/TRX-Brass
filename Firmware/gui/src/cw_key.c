/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "cw_key.h"
#include "generator.h"
#include "util.h"
#include "fpga/dac.h"

static generator_tone_t tone;

void cw_key_init() {
    generator_tone_set_freq(&tone, options->cw.key_tone, DAC_RATE);
}

uint16_t cw_key_change_speed(int16_t d) {
    if (d == 0) {
        return options->cw.key_speed;
    }

    options->cw.key_speed = limit(options->cw.key_speed + d, 5, 50);

    return options->cw.key_speed;
}

key_mode_t cw_key_change_mode(int16_t d) {
    if (d == 0) {
        return options->cw.key_mode;
    }

    switch (options->cw.key_mode) {
        case KEY_MODE_MANUAL:
            options->cw.key_mode = d > 0 ? KEY_MODE_AUTO_LEFT : KEY_MODE_AUTO_RIGHT;
            break;

        case KEY_MODE_AUTO_LEFT:
            options->cw.key_mode = d > 0 ? KEY_MODE_AUTO_RIGHT : KEY_MODE_MANUAL;
            break;

        case KEY_MODE_AUTO_RIGHT:
            options->cw.key_mode = d > 0 ? KEY_MODE_MANUAL : KEY_MODE_AUTO_LEFT;
            break;
    }

    return options->cw.key_mode;
}

iambic_mode_t cw_key_change_iambic_mode(int16_t d) {
    if (d == 0) {
        return options->cw.iambic_mode;
    }

    options->cw.iambic_mode = (options->cw.iambic_mode == IAMBIC_A) ? IAMBIC_B : IAMBIC_A;

    return options->cw.iambic_mode;
}

uint16_t cw_key_change_tone(int16_t d) {
    if (d == 0) {
        return options->cw.key_tone;
    }

    options->cw.key_tone = limit(options->cw.key_tone + ((d > 0) ? 10 : -10), 400, 1200);

    generator_tone_set_freq(&tone, options->cw.key_tone, DAC_RATE);

    return options->cw.key_tone;
}

bool cw_key_change_train(int16_t d) {
    if (d == 0) {
        return options->cw.key_train;
    }

    options->cw.key_train = !options->cw.key_train;

    return options->cw.key_train;
}

uint16_t cw_key_change_qsk_time(int16_t d) {
    if (d == 0) {
        return options->cw.qsk_time;
    }

    options->cw.qsk_time = limit(options->cw.qsk_time + (d > 0 ? 10: -10), 0, 1000);

    return options->cw.qsk_time;
}

uint8_t cw_key_change_ratio(int16_t d) {
    if (d == 0) {
        return options->cw.key_ratio;
    }

    options->cw.key_ratio = limit(options->cw.key_ratio + (d > 0 ? 5 : -5), 25, 45);

    return options->cw.key_ratio;
}

size_t cw_key_generator(float complex *data, size_t max_size, bool reverse) {
    size_t size = 128;

    for (uint16_t i = 0; i < size; i++) {
        data[i] = generator_tone(&tone);
    }

    return size;
}
