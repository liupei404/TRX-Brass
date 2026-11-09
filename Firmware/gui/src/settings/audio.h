/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cyaml/cyaml.h>

#include "filter.h"
#include "../dsp/eq.h"

#define EQUALIZER_NUM   3

typedef struct {
    int16_t     freq;
    int16_t     q;      /* 1 .. 10 */
    int16_t     gain;   /* -20 .. 10 */
} equalizer_item_t;

typedef struct {
    filter_t            filter;
    eq_point_t          eq[EQ_MAX_POINTS];
    char                *device;
} options_mic_t;

typedef struct {
    uint8_t             vol;
    equalizer_item_t    *eq;
    char                *device;
} options_speaker_t;

typedef enum {
    REC_FORMAT_WAV = 0,
    REC_FORMAT_MP3
} rec_format_t;

typedef enum {
    DENOISE_OFF = 0,
    DENOISE_NR,
    DENOISE_EMNR,
} denoise_mode_t;

typedef struct {
    uint16_t            frame_size;
    int16_t             reduction_amount;
    int16_t             smoothing_factor;
    int16_t             whitening_factor;
    int16_t             noise_scaling_type;
    int16_t             noise_rescale;
    int16_t             post_filter_threshold;
} options_nr_t;

typedef struct {
    uint16_t            fft;
    uint16_t            over;
    uint16_t            gain_method;
    uint16_t            npe_method;
    float               trained_thresh;
    float               trained_t2;
} options_emnr_t;

typedef struct {
    denoise_mode_t      mode;
    bool                before_agc;
    options_nr_t        nr;
    options_emnr_t      emnr;
} denoise_t;

typedef struct {
    options_speaker_t   speaker;
    options_mic_t       mic;
    rec_format_t        rec_format;
    denoise_t           denoise;
} options_audio_t;

extern const cyaml_schema_field_t audio_fields_schema[];

/* Replace an invalid microphone profile with the neutral default. */
bool settings_mic_validate(options_mic_t *mic);
eq_config_t settings_mic_eq_config(const options_mic_t *mic);
