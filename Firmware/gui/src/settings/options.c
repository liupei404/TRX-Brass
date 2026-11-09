/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <cyaml/cyaml.h>
#include "lvgl/lvgl.h"
#include "options.h"
#include "src/qth.h"
#include "src/hkey.h"
#include "src/dsp.h"
#include "src/vol.h"
#include "src/mfk.h"

static const char  filename[] = "/mnt/settings/options.yaml";

static const cyaml_schema_field_t operator_fields_schema[] = {
    CYAML_FIELD_STRING("qth",           CYAML_FLAG_OPTIONAL, options_operator_t, qth, 0),
    CYAML_FIELD_STRING("callsign",      CYAML_FLAG_OPTIONAL, options_operator_t, callsign, 0),
    CYAML_FIELD_END
};

const cyaml_schema_field_t spectrum_fields_schema[] = {
    CYAML_FIELD_FLOAT("beta",           CYAML_FLAG_OPTIONAL, options_spectrum_t, beta),
    CYAML_FIELD_BOOL("filled",          CYAML_FLAG_OPTIONAL, options_spectrum_t, filled),
    CYAML_FIELD_BOOL("peak",            CYAML_FLAG_OPTIONAL, options_spectrum_t, peak),
    CYAML_FIELD_UINT("peak_hold",       CYAML_FLAG_OPTIONAL, options_spectrum_t, peak_hold),
    CYAML_FIELD_FLOAT("peak_speed",     CYAML_FLAG_OPTIONAL, options_spectrum_t, peak_speed),
    CYAML_FIELD_END
};

static const cyaml_strval_t vol_flags_strings[] = {
    { "vol",                (1 << VOL_VOL) },
    { "pwr",                (1 << VOL_PWR) },
    { "freq_mode",          (1 << VOL_FREQ_MODE) },
    { "split",              (1 << VOL_SPLIT) },
    { "voice_lang",         (1 << VOL_VOICE_LANG) },
    { "voice_rate",         (1 << VOL_VOICE_RATE) },
    { "voice_pitch",        (1 << VOL_VOICE_PITCH) },
    { "voice_volume",       (1 << VOL_VOICE_VOLUME) },
};

static const cyaml_strval_t mfk_flags_strings[] = {
    { "filter_low",         (1 << MFK_FILTER_LOW ) },
    { "filter_high",        (1 << MFK_FILTER_HIGH )},
    { "filter_transition",  (1 << MFK_FILTER_TRANSITION )},
    { "agc",                (1 << MFK_AGC )},
    { "spectrum_factor",    (1 << MFK_SPECTRUM_FACTOR )},
    { "spectrum_beta",      (1 << MFK_SPECTRUM_BETA )},
    { "peak_hold",          (1 << MFK_PEAK_HOLD )},
    { "peak_speed",         (1 << MFK_PEAK_SPEED )},
    { "spectrum_fill",      (1 << MFK_SPECTRUM_FILL )},
    { "spectrum_peak",      (1 << MFK_SPECTRUM_PEAK )},
    { "denoise_mode",       (1 << MFK_DENOISE_MODE )},
};

static const cyaml_strval_t action_strings[] = {
    { "none",               ACTION_NONE },
    { "screenshot",         ACTION_SCREENSHOT },
    { "recorder",           ACTION_RECORDER },
    { "mute",               ACTION_MUTE },
    { "step_up",            ACTION_STEP_UP },
    { "step_down",          ACTION_STEP_DOWN },
    { "voice_mode",         ACTION_VOICE_MODE },

    { "app_rtty",           ACTION_APP_RTTY },
    { "app_ft8",            ACTION_APP_FT8 },
    { "app_swrscan",        ACTION_APP_SWRSCAN },
    { "app_gps",            ACTION_APP_GPS },
    { "app_settings",       ACTION_APP_SETTINGS },
    { "app_recorder",       ACTION_APP_RECORDER },
    { "app_qth",            ACTION_APP_QTH },
    { "app_callsign",       ACTION_APP_CALLSIGN },
    { "app_freq",           ACTION_APP_FREQ }
};

static const cyaml_strval_t key_mode_strings[] = {
    { "manual",             KEY_MODE_MANUAL },
    { "auto_left",          KEY_MODE_AUTO_LEFT },
    { "auto_right",         KEY_MODE_AUTO_RIGHT },
};

static const cyaml_strval_t iambic_mode_strings[] = {
    { "mode_a",             IAMBIC_A },
    { "mode_b",             IAMBIC_B },
};

static const cyaml_strval_t voice_mode_strings[] = {
    { "off",                VOICE_OFF },
    { "lcd",                VOICE_LCD },
    { "always",             VOICE_ALWAYS },
};

static const cyaml_strval_t freq_accel_strings[] = {
    { "none",               FREQ_ACCEL_NONE },
    { "lite",               FREQ_ACCEL_LITE },
    { "strong",             FREQ_ACCEL_STRONG },
};

static const cyaml_strval_t freq_mode_strings[] = {
    { "join",               FREQ_MODE_JOIN },
    { "slide",              FREQ_MODE_SLIDE },
    { "rx_only",            FREQ_MODE_RX_ONLY },
    { "fft_only",           FREQ_MODE_FFT_ONLY },
};

static const cyaml_strval_t clock_view_strings[] = {
    { "time_always",        CLOCK_TIME_ALWAYS },
    { "time_power",         CLOCK_TIME_POWER },
    { "power_always",       CLOCK_POWER_ALWAYS },
};

#define CONTROL_FLAGS (CYAML_FLAG_OPTIONAL | CYAML_FLAG_STRICT)

const cyaml_schema_field_t control_fields_schema[] = {
    CYAML_FIELD_FLAGS("vol",            CONTROL_FLAGS, options_control_t, vol, vol_flags_strings, CYAML_ARRAY_LEN(vol_flags_strings)),
    CYAML_FIELD_FLAGS("mfk",            CONTROL_FLAGS, options_control_t, mfk, mfk_flags_strings, CYAML_ARRAY_LEN(mfk_flags_strings)),
    CYAML_FIELD_ENUM("long_vol",        CYAML_FLAG_OPTIONAL, options_control_t, long_vol, action_strings, CYAML_ARRAY_LEN(action_strings)),
    CYAML_FIELD_ENUM("long_mfk",        CYAML_FLAG_OPTIONAL, options_control_t, long_mfk, action_strings, CYAML_ARRAY_LEN(action_strings)),
    CYAML_FIELD_ENUM("long_app",        CYAML_FLAG_OPTIONAL, options_control_t, long_app, action_strings, CYAML_ARRAY_LEN(action_strings)),
    CYAML_FIELD_ENUM("long_band_down",  CYAML_FLAG_OPTIONAL, options_control_t, long_band_down, action_strings, CYAML_ARRAY_LEN(action_strings)),
    CYAML_FIELD_ENUM("long_band_up",    CYAML_FLAG_OPTIONAL, options_control_t, long_band_up, action_strings, CYAML_ARRAY_LEN(action_strings)),
    CYAML_FIELD_END
};

const cyaml_schema_field_t rtty_fields_schema[] = {
    CYAML_FIELD_UINT("center",          CYAML_FLAG_OPTIONAL, options_rtty_t, center),
    CYAML_FIELD_UINT("shift",           CYAML_FLAG_OPTIONAL, options_rtty_t, shift),
    CYAML_FIELD_FLOAT("rate",           CYAML_FLAG_OPTIONAL, options_rtty_t, rate),
    CYAML_FIELD_BOOL("reverse",         CYAML_FLAG_OPTIONAL, options_rtty_t, reverse),
    CYAML_FIELD_UINT("bits",            CYAML_FLAG_OPTIONAL, options_rtty_t, bits),
    CYAML_FIELD_FLOAT("snr",            CYAML_FLAG_OPTIONAL, options_rtty_t, snr),
    CYAML_FIELD_END
};

const cyaml_schema_field_t cw_fields_schema[] = {
    CYAML_FIELD_BOOL("decoder",             CYAML_FLAG_OPTIONAL, options_cw_t, decoder),
    CYAML_FIELD_FLOAT("decoder_snr",        CYAML_FLAG_OPTIONAL, options_cw_t, decoder_snr),
    CYAML_FIELD_FLOAT("decoder_snr_gist",   CYAML_FLAG_OPTIONAL, options_cw_t, decoder_snr_gist),
    CYAML_FIELD_FLOAT("decoder_peak_beta",  CYAML_FLAG_OPTIONAL, options_cw_t, decoder_peak_beta),
    CYAML_FIELD_FLOAT("decoder_noise_beta", CYAML_FLAG_OPTIONAL, options_cw_t, decoder_noise_beta),
    CYAML_FIELD_UINT("key_speed",           CYAML_FLAG_OPTIONAL, options_cw_t, key_speed),
    CYAML_FIELD_UINT("key_ratio",           CYAML_FLAG_OPTIONAL, options_cw_t, key_ratio),
    CYAML_FIELD_UINT("key_tone",            CYAML_FLAG_OPTIONAL, options_cw_t, key_tone),
    CYAML_FIELD_ENUM("key_mode",            CYAML_FLAG_OPTIONAL, options_cw_t, key_mode, key_mode_strings, CYAML_ARRAY_LEN(key_mode_strings)),
    CYAML_FIELD_BOOL("key_train",           CYAML_FLAG_OPTIONAL, options_cw_t, key_train),
    CYAML_FIELD_UINT("qsk_time",            CYAML_FLAG_OPTIONAL, options_cw_t, qsk_time),
    CYAML_FIELD_ENUM("iambic_mode",         CYAML_FLAG_OPTIONAL, options_cw_t, iambic_mode, iambic_mode_strings, CYAML_ARRAY_LEN(iambic_mode_strings)),
    CYAML_FIELD_END
};

const cyaml_schema_field_t olivia_fields_schema[] = {
    CYAML_FIELD_UINT("tones",               CYAML_FLAG_OPTIONAL, options_olivia_t, tones),
    CYAML_FIELD_UINT("band_width",          CYAML_FLAG_OPTIONAL, options_olivia_t, band_width),
    CYAML_FIELD_UINT("band_lower",          CYAML_FLAG_OPTIONAL, options_olivia_t, band_lower),
    CYAML_FIELD_UINT("sync_margin",         CYAML_FLAG_OPTIONAL, options_olivia_t, sync_margin),
    CYAML_FIELD_UINT("sync_integ",          CYAML_FLAG_OPTIONAL, options_olivia_t, sync_integ),
    CYAML_FIELD_FLOAT("sync_threshold",     CYAML_FLAG_OPTIONAL, options_olivia_t, sync_threshold),
    CYAML_FIELD_END
};

const cyaml_schema_field_t msg_fields_schema[] = {
    CYAML_FIELD_UINT("voice_period",        CYAML_FLAG_OPTIONAL, options_msg_t, voice_period),
    CYAML_FIELD_UINT("cw_period",           CYAML_FLAG_OPTIONAL, options_msg_t, cw_period),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t action_entry = {
    CYAML_VALUE_ENUM(CYAML_FLAG_DEFAULT, options_action_t, action_strings, CYAML_ARRAY_LEN(action_strings)),
};

const cyaml_schema_field_t hkeys_fields_schema[] = {
    CYAML_FIELD_STRING_PTR("hmic",              CYAML_FLAG_POINTER,  options_hkeys_t, hmic, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE_FIXED("press_p",       CYAML_FLAG_OPTIONAL, options_hkeys_t, press_p, &action_entry, 4),
    CYAML_FIELD_SEQUENCE_FIXED("press_char",    CYAML_FLAG_OPTIONAL, options_hkeys_t, press_char, &action_entry, 4),
    CYAML_FIELD_END
};

const cyaml_schema_field_t mag_fields_schema[] = {
    CYAML_FIELD_BOOL("freq",                    CYAML_FLAG_OPTIONAL, options_mag_t, freq),
    CYAML_FIELD_BOOL("info",                    CYAML_FLAG_OPTIONAL, options_mag_t, info),
    CYAML_FIELD_BOOL("alc",                     CYAML_FLAG_OPTIONAL, options_mag_t, alc),
    CYAML_FIELD_END
};

const cyaml_schema_field_t voice_fields_schema[] = {
    CYAML_FIELD_ENUM("mode",                    CYAML_FLAG_OPTIONAL, options_voice_t, mode, voice_mode_strings, CYAML_ARRAY_LEN(voice_mode_strings)),
    CYAML_FIELD_UINT("lang",                    CYAML_FLAG_OPTIONAL, options_voice_t, lang),
    CYAML_FIELD_UINT("rate",                    CYAML_FLAG_OPTIONAL, options_voice_t, rate),
    CYAML_FIELD_UINT("pitch",                   CYAML_FLAG_OPTIONAL, options_voice_t, pitch),
    CYAML_FIELD_UINT("volume",                  CYAML_FLAG_OPTIONAL, options_voice_t, volume),
    CYAML_FIELD_END
};

const cyaml_schema_field_t freq_fields_schema[] = {
    CYAML_FIELD_ENUM("accel",                   CYAML_FLAG_OPTIONAL, options_freq_t, accel, freq_accel_strings, CYAML_ARRAY_LEN(freq_accel_strings)),
    CYAML_FIELD_ENUM("mode",                    CYAML_FLAG_OPTIONAL, options_freq_t, mode, freq_mode_strings, CYAML_ARRAY_LEN(freq_mode_strings)),
    CYAML_FIELD_END
};

const cyaml_schema_field_t clock_fields_schema[] = {
    CYAML_FIELD_ENUM("view",                    CYAML_FLAG_OPTIONAL, options_clock_t, view, clock_view_strings, CYAML_ARRAY_LEN(clock_view_strings)),
    CYAML_FIELD_UINT("time_timeout",            CYAML_FLAG_OPTIONAL, options_clock_t, time_timeout),
    CYAML_FIELD_UINT("power_timeout",           CYAML_FLAG_OPTIONAL, options_clock_t, power_timeout),
    CYAML_FIELD_UINT("tx_timeout",              CYAML_FLAG_OPTIONAL, options_clock_t, tx_timeout),
    CYAML_FIELD_END
};

const cyaml_schema_field_t options_fields_schema[] = {
    CYAML_FIELD_MAPPING("operator",     CYAML_FLAG_OPTIONAL, options_t, op, operator_fields_schema),
    CYAML_FIELD_MAPPING("audio",        CYAML_FLAG_OPTIONAL, options_t, audio, audio_fields_schema),
    CYAML_FIELD_MAPPING("spectrum",     CYAML_FLAG_OPTIONAL, options_t, spectrum, spectrum_fields_schema),
    CYAML_FIELD_MAPPING("control",      CYAML_FLAG_OPTIONAL, options_t, control, control_fields_schema),
    CYAML_FIELD_MAPPING("rtty",         CYAML_FLAG_OPTIONAL, options_t, rtty, rtty_fields_schema),
    CYAML_FIELD_MAPPING("cw",           CYAML_FLAG_OPTIONAL, options_t, cw, cw_fields_schema),
    CYAML_FIELD_MAPPING("olivia",       CYAML_FLAG_OPTIONAL, options_t, olivia, olivia_fields_schema),
    CYAML_FIELD_MAPPING("msg",          CYAML_FLAG_OPTIONAL, options_t, msg, msg_fields_schema),
    CYAML_FIELD_MAPPING("hkeys",        CYAML_FLAG_OPTIONAL, options_t, hkeys, hkeys_fields_schema),
    CYAML_FIELD_MAPPING("mag",          CYAML_FLAG_OPTIONAL, options_t, mag, mag_fields_schema),
    CYAML_FIELD_MAPPING("voice",        CYAML_FLAG_OPTIONAL, options_t, voice, voice_fields_schema),
    CYAML_FIELD_MAPPING("freq",         CYAML_FLAG_OPTIONAL, options_t, freq, freq_fields_schema),
    CYAML_FIELD_MAPPING("clock",        CYAML_FLAG_OPTIONAL, options_t, clock, clock_fields_schema),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t options_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, options_t, options_fields_schema)
};

static const cyaml_config_t config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

options_t   *options = NULL;

void settings_options_load() {
    cyaml_err_t err;

    err = cyaml_load_file(filename, &config, &options_schema, (void **) &options, NULL);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }

    if (!settings_mic_validate(&options->audio.mic)) {
        LV_LOG_WARN("Invalid microphone filter/EQ; using neutral defaults");
    }

    qth_update(options->op.qth);
    hkey_mic_update();
}

void settings_options_save() {
    cyaml_err_t err;

    err = cyaml_save_file(filename, &config, &options_schema, options, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}
