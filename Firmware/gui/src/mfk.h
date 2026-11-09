/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>

typedef enum {
    MFK_FILTER_LOW = 0,
    MFK_FILTER_HIGH,
    MFK_FILTER_TRANSITION,
    MFK_AGC,

    MFK_SPECTRUM_FACTOR,
    MFK_SPECTRUM_BETA,
    MFK_PEAK_HOLD,
    MFK_PEAK_SPEED,
    MFK_SPECTRUM_FILL,
    MFK_SPECTRUM_PEAK,

    MFK_KEY_SPEED,
    MFK_KEY_MODE,
    MFK_IAMBIC_MODE,
    MFK_KEY_TONE,
    MFK_KEY_VOL,
    MFK_KEY_TRAIN,
    MFK_QSK_TIME,
    MFK_KEY_RATIO,

    MFK_DNF,
    MFK_DNF_CENTER,
    MFK_DNF_WIDTH,
    MFK_NB,
    MFK_NB_LEVEL,
    MFK_NB_WIDTH,
    MFK_DENOISE_MODE,

    MFK_CW_DECODER,
    MFK_CW_DECODER_SNR,
    MFK_CW_DECODER_PEAK_BETA,
    MFK_CW_DECODER_NOISE_BETA,

    MFK_ANT,

    MFK_LAST,

    /* APPs */

    MFK_RTTY_RATE,
    MFK_RTTY_SHIFT,
    MFK_RTTY_CENTER,
    MFK_RTTY_REVERSE,

    MFK_OLIVIA_TONES,
    MFK_OLIVIA_WIDTH
} mfk_mode_t;

typedef enum {
    MFK_STATE_EDIT = 0,
    MFK_STATE_SELECT
} mfk_state_t;

extern mfk_state_t  mfk_state;

void mfk_update(int16_t diff, bool voice);
void mfk_press(int16_t dir);
void mfk_set_mode(mfk_mode_t mode);
