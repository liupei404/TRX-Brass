/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "filter.h"
#include "audio.h"

typedef struct {
    char                qth[16];
    char                callsign[16];
} options_operator_t;

typedef struct {
    float               beta;
    bool                filled;
    bool                peak;
    uint16_t            peak_hold;
    float               peak_speed;
} options_spectrum_t;

typedef enum {
    ACTION_NONE = 0,
    ACTION_SCREENSHOT,
    ACTION_RECORDER,
    ACTION_MUTE,
    ACTION_STEP_UP,
    ACTION_STEP_DOWN,
    ACTION_VOICE_MODE,

    ACTION_APP_RTTY = 100,
    ACTION_APP_FT8,
    ACTION_APP_SWRSCAN,
    ACTION_APP_GPS,
    ACTION_APP_SETTINGS,
    ACTION_APP_RECORDER,
    ACTION_APP_QTH,
    ACTION_APP_CALLSIGN,
    ACTION_APP_FREQ
} options_action_t;

typedef struct {
    uint64_t            vol;
    uint64_t            mfk;
    options_action_t    long_vol;
    options_action_t    long_mfk;
    options_action_t    long_app;
    options_action_t    long_band_down;
    options_action_t    long_band_up;
} options_control_t;

typedef struct {
    uint16_t            center;
    uint16_t            shift;
    float               rate;
    bool                reverse;
    uint8_t             bits;
    float               snr;
} options_rtty_t;

typedef enum {
    KEY_MODE_MANUAL = 0,
    KEY_MODE_AUTO_LEFT,
    KEY_MODE_AUTO_RIGHT
} key_mode_t;

typedef enum {
    IAMBIC_A = 0,
    IAMBIC_B
} iambic_mode_t;

typedef struct {
    bool                decoder;
    float               decoder_snr;
    float               decoder_snr_gist;
    float               decoder_peak_beta;
    float               decoder_noise_beta;
    uint8_t             key_speed;
    uint8_t             key_ratio;
    uint16_t            key_tone;
    key_mode_t          key_mode;
    bool                key_train;
    uint16_t            qsk_time;
    iambic_mode_t       iambic_mode;
} options_cw_t;

typedef struct {
    uint16_t            tones;
    uint16_t            band_width;
    uint16_t            band_lower;
    uint16_t            sync_margin;
    uint16_t            sync_integ;
    float               sync_threshold;
} options_olivia_t;

typedef struct {
    uint16_t            voice_period;
    uint16_t            cw_period;
} options_msg_t;

typedef struct {
    char                *hmic;
    options_action_t    press_p[4];
    options_action_t    press_char[4];
} options_hkeys_t;

typedef struct {
    bool                freq;
    bool                info;
    bool                alc;
} options_mag_t;

typedef enum {
    VOICE_OFF = 0,
    VOICE_LCD,
    VOICE_ALWAYS
} voice_mode_t;

typedef struct {
    voice_mode_t        mode;
    uint8_t             lang;
    uint8_t             rate;
    uint8_t             pitch;
    uint8_t             volume;
} options_voice_t;

typedef enum {
    FREQ_ACCEL_NONE = 0,
    FREQ_ACCEL_LITE,
    FREQ_ACCEL_STRONG
} freq_accel_t;

typedef enum {
    FREQ_MODE_JOIN = 0,
    FREQ_MODE_SLIDE,
    FREQ_MODE_RX_ONLY,
    FREQ_MODE_FFT_ONLY
} freq_mode_t;

typedef struct {
    freq_accel_t        accel;
    freq_mode_t         mode;
} options_freq_t;

typedef enum {
    CLOCK_TIME_ALWAYS = 0,
    CLOCK_TIME_POWER,
    CLOCK_POWER_ALWAYS
} clock_view_t;

typedef struct {
    clock_view_t        view;
    uint8_t             time_timeout;
    uint8_t             power_timeout;
    uint8_t             tx_timeout;
} options_clock_t;

typedef struct {
    options_operator_t  op;
    options_audio_t     audio;
    options_spectrum_t  spectrum;
    options_control_t   control;
    options_rtty_t      rtty;
    options_cw_t        cw;
    options_olivia_t    olivia;
    options_msg_t       msg;
    options_hkeys_t     hkeys;
    options_mag_t       mag;
    options_voice_t     voice;
    options_freq_t      freq;
    options_clock_t     clock;
} options_t;

extern options_t   *options;

void settings_options_load();
void settings_options_save();
