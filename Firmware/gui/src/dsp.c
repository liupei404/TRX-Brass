/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <specbleach_adenoiser.h>

#include "smeter.h"
#include "dsp.h"
#include "dmr.h"
#include "util.h"
#include "radio.h"
#include "audio.h"
#include "cw.h"
#include "cw_key.h"
#include "rtty.h"
#include "dialog_ft8.h"
#include "dialog_msg_voice.h"
#include "recorder.h"
#include "fpga/mb_data.h"
#include "fpga/adc.h"
#include "fpga/dac.h"
#include "fpga/control.h"
#include "dsp/firdes.h"
#include "dsp/agc.h"
#include "msgs.h"
#include "generator.h"
#include "mic.h"
#include "main.h"
#include "settings/modes.h"
#include "settings/options.h"
#include "settings/rf.h"
#include "settings/op_work.h"
#include "olivia/olivia.h"
#include "dsp/biquad.h"
#include "dsp/emnr.h"
#include "two_tone.h"

const uint16_t                  fft_over = (FFT_SAMPLES - 800) / 2;

static float                    fft_correct_db = 85.0f;
static float                    meter_correct_db = 50.0f;

static float                    *spectrum_psd;
static uint16_t                 spectrum_psd_count = 0;
static msgs_auto_t              spectrum_auto_msg;
static msgs_floats_t            spectrum_data_msg;
static lv_timer_t               *spectrum_timer = NULL;
static pthread_mutex_t          spectrum_mux;

static float                    *waterfall_psd;
static uint16_t                 waterfall_psd_count = 0;
static lv_timer_t               *waterfall_timer = NULL;
static msgs_auto_t              waterfall_auto_msg;
static pthread_mutex_t          waterfall_mux;
static msgs_floats_t            waterfall_data_msg;

static firhilbf                 demod_ssb;
static firfilt_rrrf             demod_dc_block;
static agc_t                    *rx_agc;

static firhilbf                 mod_ssb;

static size_t                   filter_len = 0;
static float                    *filter_taps = NULL;
static bool                     filter_need_update = false;
static firfilt_rrrf             filter = NULL;

static float                    *auto_psd;
static uint16_t                 auto_psd_count = 0;
static lv_timer_t               *auto_timer = NULL;
static pthread_mutex_t          auto_mux;

static uint8_t                  meter_count = 0;
static float                    meter_sum_db = 0.0f;
static float                    meter_avr_db = 0.0f;
static lv_timer_t               *meter_timer = NULL;
static pthread_mutex_t          meter_mux;

static uint8_t                  delay;

static bool                     ready = false;
static bool                     auto_clear = true;

static float                    fft_buf[FFT_SAMPLES];

static int16_t                  adc_buf[ADC_SAMPLES];
static float                    adc_vol = 0;
static bool                     adc_mute = false;

static bool                     adc_equalizer_update = true;
static biquad_t                 adc_equalizer[EQUALIZER_NUM];

static float                    audio_buf[ADC_SAMPLES];
static float                    denoised_buf[ADC_SAMPLES];
static SpectralBleachHandle     denoise;
static bool                     denoise_need_update = true;
static emnr_t                   *emnr;

static void calc_auto();
static void spectrum_timer_cb(lv_timer_t *t);
static void waterfall_timer_cb(lv_timer_t *t);
static void auto_timer_cb(lv_timer_t *t);
static void meter_timer_cb(lv_timer_t *t);

/* * */

static float dB(float x) {
    if (x > 0.0f) {
        return 10.0f * log10f(x) - fft_correct_db;
    } else {
        return S_MIN;
    }
}

/* * */

void dsp_init() {
    pthread_mutex_init(&spectrum_mux, NULL);
    pthread_mutex_init(&waterfall_mux, NULL);
    pthread_mutex_init(&auto_mux, NULL);
    pthread_mutex_init(&meter_mux, NULL);

    spectrum_data_msg.size = 800;
    spectrum_data_msg.data = (float *) malloc(spectrum_data_msg.size * sizeof(float));

    for (uint16_t i = 0; i < spectrum_data_msg.size; i++)
        spectrum_data_msg.data[i] = S_MIN;

    spectrum_psd = (float *) malloc(spectrum_data_msg.size * sizeof(float));

    waterfall_psd = (float *) malloc(FFT_SAMPLES * sizeof(float));
    auto_psd = (float *) malloc(FFT_SAMPLES * sizeof(float));

    waterfall_data_msg.size = FFT_SAMPLES;
    waterfall_data_msg.data = waterfall_psd;

    demod_ssb = firhilbf_create(15, 60.0f);
    demod_dc_block = firfilt_rrrf_create_dc_blocker(25, 20.0f);

    mod_ssb = firhilbf_create(15, 60.0f);

    rx_agc = agc_create(
        AGC_FAST,   /* mode */
        ADC_RATE,   /* sample rate */
        0.001f,     /* tau_attack */
        0.250f,     /* tau_decay */
        4,          /* n_tau */
        100.0f,     /* max_gain */
        1.5f,       /* var_gain */
        4.0f,       /* fixed_gain */
        1.0f,       /* max_input */
        1.0f,       /* out_target */
        0.250f,     /* tau_fast_backaverage */
        0.005f,     /* tau_fast_decay */
        5.0f,       /* pop_ratio */
        1,          /* hang_enable */
        0.500f,     /* tau_hang_backmult */
        0.250f,     /* hangtime */
        0.250f,     /* hang_thresh */
        0.100f      /* tau_hang_decay */
    );

    delay = 4;

    dsp_set_vol(options->audio.speaker.vol);
    ready = true;

    spectrum_timer = lv_timer_create(spectrum_timer_cb, 1000 / 20, NULL);
    waterfall_timer = lv_timer_create(waterfall_timer_cb, 1000 / 10, NULL);
    auto_timer = lv_timer_create(auto_timer_cb, 1000 / 10, NULL);
    meter_timer = lv_timer_create(meter_timer_cb, 1000 / 10, NULL);

    denoise = specbleach_adaptive_initialize(ADC_RATE, options->audio.denoise.nr.frame_size);

    emnr = emnr_create(
        32,             /* samples */
        audio_buf,
        denoised_buf,
        options->audio.denoise.emnr.fft,
        options->audio.denoise.emnr.over,
        ADC_RATE,
        0,
        1.0,
        options->audio.denoise.emnr.gain_method,
        options->audio.denoise.emnr.npe_method,
        1
    );

    if (!dmr_init())
        LV_LOG_ERROR("Cannot initialize DMR DSP");

    two_tone_update();
}

void dsp_reset() {
    dmr_reset();
    delay = 4;
}

void dsp_set_filter(filter_t *filter) {
    dmr_reset();
    size_t len = firdes_compute_taps_len(ADC_RATE, filter->transition, 40.0f);

    if (filter_len != len) {
        filter_taps = realloc(filter_taps, len * sizeof(float));
        filter_len = len;
    }

    firdes_band_pass(1.0f, ADC_RATE, filter->low, filter->high, filter_taps, filter_len);
    filter_need_update = true;

    brass_msg_send(MSG_FILTER_CHANGED, NULL);
}

void dsp_set_rx_agc(uint8_t mode) {
    agc_set_mode(rx_agc, mode);
    brass_msg_send(MSG_AGC_CHANGED, &mode);
}

int dsp_change_denoise(int16_t d) {
    if (d == 0) {
        return options->audio.denoise.mode;
    }

    options->audio.denoise.mode = limit(options->audio.denoise.mode + d, DENOISE_OFF, DENOISE_EMNR);

    return options->audio.denoise.mode;
}

void dsp_update_denoise() {
    denoise_need_update = true;
}

uint8_t dsp_change_rx_agc(int16_t df) {
    if (df == 0) {
        return op_mode->agc;
    }

    op_mode->agc = limit(op_mode->agc + df, AGC_OFF, AGC_CUSTOM);

    dsp_set_rx_agc(op_mode->agc);

    return op_mode->agc;
}

static void auto_timer_cb(lv_timer_t *t) {
    if (auto_psd_count == 0) {
        return;
    }

    pthread_mutex_lock(&auto_mux);

    for (size_t i = 0; i < FFT_SAMPLES; i++) {
        auto_psd[i] /= auto_psd_count;
    }

    calc_auto();
    auto_psd_count = 0;
    memset(auto_psd, 0, FFT_SAMPLES * sizeof(float));

    pthread_mutex_unlock(&auto_mux);
}

static void spectrum_timer_cb(lv_timer_t *t) {
    if (spectrum_psd_count == 0) {
        return;
    }

    pthread_mutex_lock(&spectrum_mux);

    for (uint16_t i = 0; i < spectrum_data_msg.size; i++) {
        float x = spectrum_psd[i] / spectrum_psd_count;

        lpf(&spectrum_data_msg.data[i], dB(x), options->spectrum.beta);
    }

    brass_msg_send(MSG_SPECTRUM_DATA, &spectrum_data_msg);

    spectrum_psd_count = 0;
    memset(spectrum_psd, 0, spectrum_data_msg.size * sizeof(float));

    pthread_mutex_unlock(&spectrum_mux);
}

static void waterfall_timer_cb(lv_timer_t *t) {
    if (waterfall_psd_count == 0) {
        return;
    }

    pthread_mutex_lock(&waterfall_mux);

    for (uint16_t i = 0; i < FFT_SAMPLES; i++) {
        float x = waterfall_psd[i] / waterfall_psd_count;

        waterfall_psd[i] = dB(x);
    }

    brass_msg_send(MSG_WATERFALL_DATA, &waterfall_data_msg);

    waterfall_psd_count = 0;
    memset(waterfall_psd, 0, FFT_SAMPLES * sizeof(float));

    pthread_mutex_unlock(&waterfall_mux);
}

void dsp_fft(float *data) {
    radio_state_t   state = radio_get_state();

    if (state != RADIO_RX) {
        return;
    }

    for (size_t i = 0; i < FFT_SAMPLES; i++) {
        uint16_t    index = (i + FFT_SAMPLES / 2) % FFT_SAMPLES;

        fft_buf[index] = data[i];
    }

    pthread_mutex_lock(&waterfall_mux);

    for (size_t i = 0; i < FFT_SAMPLES; i++)
        waterfall_psd[i] += fft_buf[i];

    waterfall_psd_count++;

    pthread_mutex_unlock(&waterfall_mux);
    pthread_mutex_lock(&auto_mux);

    for (size_t i = 0; i < FFT_SAMPLES; i++)
        auto_psd[i] += fft_buf[i];

    auto_psd_count++;

    pthread_mutex_unlock(&auto_mux);
    pthread_mutex_lock(&spectrum_mux);

    for (size_t i = 0; i < spectrum_data_msg.size; i++)
        spectrum_psd[i] += fft_buf[i + fft_over];

    spectrum_psd_count++;
    pthread_mutex_unlock(&spectrum_mux);
}

float complex dsp_modulate(float x, radio_mode_t mode) {
    static float phase = 0;

    float complex b;
    float complex out = 0;

    switch (mode) {
        case RADIO_MODE_DMR:
            dmr_tx_audio(x);
            break;

        case RADIO_MODE_USB:
            firhilbf_r2c_execute(mod_ssb, x, &b);
            __real__ out = __real__ b;
            __imag__ out = __imag__ b;
            break;

        case RADIO_MODE_LSB:
            firhilbf_r2c_execute(mod_ssb, x, &b);
            __real__ out = __imag__ b;
            __imag__ out = __real__ b;
            break;

        case RADIO_MODE_NFM:
            phase += x * 0.5f;
            out = cexpf(_Complex_I * phase);
            break;

        case RADIO_MODE_AM:
            out = _Complex_I * (x * 0.3f + 0.7f);
            break;

        default:
            break;
    }

    return out;
}

float dsp_demodulate(float complex in, radio_mode_t mode) {
    static float    last_phase = 0;
    float           phase, dphase;
    float           a, b;
    float           out = 0.0f;

    switch (mode) {
        case RADIO_MODE_LSB:
        case RADIO_MODE_CWR:
            firhilbf_c2r_execute(demod_ssb, in, &a, &b);
            out = a;
            break;

        case RADIO_MODE_USB:
        case RADIO_MODE_CW:
        case RADIO_MODE_RTTY:
        case RADIO_MODE_OLIVIA:
            firhilbf_c2r_execute(demod_ssb, in, &a, &b);
            out = b;
            break;

        case RADIO_MODE_AM:
            firfilt_rrrf_push(demod_dc_block, cabsf(in));
            firfilt_rrrf_execute(demod_dc_block, &out);
            break;

        case RADIO_MODE_NFM:
            phase = atan2f(cimagf(in), crealf(in));
            dphase = phase - last_phase;
            while (dphase < -M_PI) dphase += 2 * M_PI;
            while (dphase > M_PI) dphase -= 2 * M_PI;
            out = dphase / M_PI;
            last_phase = phase;
            break;
    }

    return out;
}

static void meter_timer_cb(lv_timer_t *t) {
    pthread_mutex_lock(&meter_mux);

    if (radio_get_state() == RADIO_RX) {
        lpf(&meter_avr_db, meter_sum_db / meter_count, 0.5f);

        brass_msg_send(MSG_SMETER, &meter_avr_db);
    }

    meter_count = 0;
    meter_sum_db = 0;

    pthread_mutex_unlock(&meter_mux);
}

void dsp_update_equalizer() {
    adc_equalizer_update = true;
}

void dsp_adc(float complex *data, uint16_t samples) {
    radio_state_t   state = radio_get_state();

    if (state != RADIO_RX) {
        dmr_receive(false, NULL, NULL, 0);
        return;
    }

    radio_mode_t    mode = op_work->mode;
    float           peak = 0.0f;

    dmr_receive(mode == RADIO_MODE_DMR, data, audio_buf, samples);

    if (mode == RADIO_MODE_DMR) {
        for (size_t i = 0; i < samples; i++) {
            float level = cabsf(data[i]);
            float value = audio_buf[i] * 32768.0f * adc_vol;

            if (level > peak)
                peak = level;

            adc_buf[i] = (int16_t)fmaxf(-32768, fminf(32767, value));
        }

        audio_adc_play(adc_buf, samples);

        if (recorder_is_on())
            recorder_put_audio_samples(audio_buf, samples);

        pthread_mutex_lock(&meter_mux);
        meter_sum_db += 20.0f * log10f(fmaxf(peak, 1e-12f)) - meter_correct_db;
        meter_count++;
        pthread_mutex_unlock(&meter_mux);

        return;
    }

    if (filter_need_update) {
        if (filter) {
            filter = firfilt_rrrf_recreate(filter, filter_taps, filter_len);
        } else {
            filter = firfilt_rrrf_create(filter_taps, filter_len);
        }
        filter_need_update = false;
    }

    if (adc_equalizer_update) {
        for (int i = 0; i < EQUALIZER_NUM; i++) {
            const equalizer_item_t *item = &options->audio.speaker.eq[i];

            biquad_peak_eq(&adc_equalizer[i], item->freq, item->q * 0.5f, item->gain, ADC_RATE);
        }

        adc_equalizer_update = false;
    }

    float x, y;

    for (int i = 0; i < samples; i++) {
        x = dsp_demodulate(data[i], mode);

        for (int n = 0; n < EQUALIZER_NUM; n++) {
            x = biqiad_apply(&adc_equalizer[n], x);
        }

        firfilt_rrrf_execute_one(filter, x, &y);

        if (fabs(y) > peak) {
            peak = fabs(y);
        }

        if (!options->audio.denoise.before_agc) {
            y = agc_apply(rx_agc, y);
        }

        audio_buf[i] = y;
    }

    if (denoise_need_update) {
        SpectralBleachParameters parameters;

        parameters.residual_listen = false;
        parameters.reduction_amount = options->audio.denoise.nr.reduction_amount;
        parameters.smoothing_factor = options->audio.denoise.nr.smoothing_factor;
        parameters.whitening_factor = options->audio.denoise.nr.whitening_factor;
        parameters.noise_scaling_type = options->audio.denoise.nr.noise_scaling_type;
        parameters.noise_rescale = options->audio.denoise.nr.noise_rescale;
        parameters.post_filter_threshold = options->audio.denoise.nr.post_filter_threshold;

        specbleach_adaptive_load_parameters(denoise, parameters);

        emnr_set_gain_method(emnr, options->audio.denoise.emnr.gain_method);
        emnr_set_npe_method(emnr, options->audio.denoise.emnr.npe_method);
        emnr_set_trained_thresh(emnr, options->audio.denoise.emnr.trained_thresh);
        emnr_set_trained_t2(emnr, options->audio.denoise.emnr.trained_t2);

        denoise_need_update = false;
    }

    float *out_buf;

    switch (options->audio.denoise.mode) {
        case DENOISE_OFF:
            out_buf = audio_buf;
            break;

        case DENOISE_NR:
            specbleach_adaptive_process(denoise, samples, audio_buf, denoised_buf);
            out_buf = denoised_buf;
            break;

        case DENOISE_EMNR:
            emnr_apply(emnr);
            out_buf = denoised_buf;
            break;
    }

    for (int i = 0; i < samples; i++) {
        x = out_buf[i];

        if (options->audio.denoise.before_agc) {
            x = agc_apply(rx_agc, x);
        }

        y = x * 16384.0f;

        if (y > 16384.0f) {
            y = 16384.0f;
        } else if (y < -16384.0f) {
            y = -16384.0f;
        }

        y *= adc_vol;

        if (y > 16384.0f) {
            y = 16384.0f;
        } else if (y < -16384.0f) {
            y = -16384.0f;
        }

        adc_buf[i] = (int16_t) y;
    }

    audio_adc_play(adc_buf, samples);

    if (recorder_is_on()) {
        recorder_put_audio_samples(out_buf, samples);
    }

    pthread_mutex_lock(&meter_mux);
    meter_sum_db += 20.0f * log10f(peak) - meter_correct_db;
    meter_count++;
    pthread_mutex_unlock(&meter_mux);

    switch (mode) {
        case RADIO_MODE_CW:
        case RADIO_MODE_CWR:
            cw_put_audio_samples(data, samples);
            break;

        case RADIO_MODE_RTTY:
            rtty_put_audio_samples(data, samples);
            break;

        case RADIO_MODE_OLIVIA:
            olivia_put_audio_samples(adc_buf, samples);
            break;

        default:
            dialog_audio_samples(data, samples);
            break;
    }
}

void dsp_set_spectrum_factor(uint8_t x) {
    control_set_fft_rate(240 * x);
    brass_msg_send(MSG_RATE_FFT_CHANGED, &x);
}

void dsp_auto_clear() {
    auto_clear = true;
}

static int compare_fft(const void *p1, const void *p2) {
    float *i1 = (float *) p1;
    float *i2 = (float *) p2;

    return (*i1 < *i2) ? -1 : 1;
}

static void calc_auto() {
    float       min = 0;
    float       max = 0;
    size_t      window = 30;
    float       *view = auto_psd + fft_over;

    qsort(view, 800, sizeof(float), compare_fft);

    for (size_t i = 0; i < window; i++) {
        min += view[i];
        max += view[800 - i - 1];
    }

    min /= window;
    max /= window;

    min = dB(min);
    max = dB(max);

    if (min < S_MIN) {
        min = S_MIN;
    }

    if (max > S9_40) {
        max = S9_40;
    }

    if (max - min < 18.0f) {
        max += 18.0f;
    }

    if (auto_clear) {
        spectrum_auto_msg.min = min;
        spectrum_auto_msg.max = max;

        waterfall_auto_msg.min = min;
        waterfall_auto_msg.max = max;

        auto_clear = false;
    } else {
        lpf(&spectrum_auto_msg.min, min, 0.7f);
        lpf(&spectrum_auto_msg.max, max, 0.7f);

        lpf(&waterfall_auto_msg.min, min, 0.4f);
        lpf(&waterfall_auto_msg.max, max, 0.4f);
    }

    brass_msg_send(MSG_SPECTRUM_AUTO, &spectrum_auto_msg);
    brass_msg_send(MSG_WATERFALL_AUTO, &waterfall_auto_msg);
}

void dsp_change_mute() {
    adc_mute = !adc_mute;
    dsp_set_vol(adc_mute ? 0 : options->audio.speaker.vol);
}

void dsp_set_mute(bool on) {
    adc_mute = on;
    dsp_set_vol(on ? 0 : options->audio.speaker.vol);
}

void dsp_set_vol(uint8_t x) {
    adc_vol = x / 100.0f;
}

uint16_t dsp_change_vol(int16_t df) {
    if (df == 0) {
        return options->audio.speaker.vol;
    }

    adc_mute = false;

    options->audio.speaker.vol = limit(options->audio.speaker.vol + df, 0, 100);
    dsp_set_vol(options->audio.speaker.vol);

    return options->audio.speaker.vol;
}

size_t dsp_dac(float complex *data, size_t max_size) {
    radio_state_t   state = radio_get_state();
    size_t          size = 0;

    if (state == RADIO_RX) {
        dmr_tx_set_active(false);
        mic_on_air(false);
    } else {
        radio_mode_t mode = op_work->mode;

        switch (rf->mode) {
            case RF_TWO_TONE:
                mode = RADIO_MODE_TWO_TONE;
                break;

            case RF_SILENCE:
                mode = RADIO_MODE_SILENCE;
                break;

            default:
                break;
        }

        dmr_tx_set_active(mode == RADIO_MODE_DMR && state == RADIO_TX);

        switch (mode) {
            case RADIO_MODE_DMR:
                size = dmr_transmit(data, max_size);
                break;

            case RADIO_MODE_CW:
                mic_on_air(false);
                size = cw_key_generator(data, max_size, false);
                break;

            case RADIO_MODE_CWR:
                mic_on_air(false);
                size = cw_key_generator(data, max_size, true);
                break;

            case RADIO_MODE_LSB:
            case RADIO_MODE_USB:
            case RADIO_MODE_AM:
            case RADIO_MODE_NFM:
                if (dialog_modulate_state()) {
                    mic_on_air(false);
                    size = dialog_modulate(data, max_size, mode);
                } else {
                    mic_on_air(true);
                    size = mic_modulate(data, max_size, mode);
                }
                break;

            case RADIO_MODE_TWO_TONE:
                mic_on_air(false);
                size = two_tone_generator(data, max_size);
                break;

            case RADIO_MODE_SILENCE:
                mic_on_air(false);
                break;

            default:
                mic_on_air(false);
                break;
        }

        if (size > max_size) {
            radio_stop_tx();
        }
    }

    float scale = sqrtf(rf->pwr / rf->ref_pwr);

    for (size_t i = 0; i < size; i++)
        data[i] *= scale;

    return size;
}
