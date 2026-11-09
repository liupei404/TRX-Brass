/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <liquid/liquid.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>

#include "mic.h"
#include "dsp.h"
#include "audio.h"
#include "msgs.h"
#include "util.h"
#include "fpga/dac.h"
#include "dsp/eq.h"
#include "dsp/agc.h"
#include "dialog_msg_voice.h"
#include "settings/options.h"

#define DECIM 441
#define INTER 128
#define OUT_SIZE (DAC_RATE / 4)

_Static_assert(DAC_RATE * DECIM == AUDIO_CAPTURE_RATE * INTER, "resampler ratio");

static _Atomic bool         on_air;
static _Atomic unsigned     stream_epoch;

static bool                 enabled;
static eq_t                 *equalizer;
static agc_t                *agc;
static rresamp_rrrf         resamp, record_resamp;
static float                input[DECIM];
static size_t               input_fill;
static unsigned             audio_epoch;
static bool                 was_active, was_recording;

/* The capture thread owns DSP state. Only the bounded output queue is shared
 * with the modulation thread; never hold its mutex during DSP or file I/O. */

static float                output[OUT_SIZE];
static size_t               output_read, output_count;
static pthread_mutex_t      output_mux = PTHREAD_MUTEX_INITIALIZER;
static float                meter_sum, meter_avr;
static size_t               meter_count;
static pthread_mutex_t      meter_mux = PTHREAD_MUTEX_INITIALIZER;
static uint64_t             config_update = 0;
static pthread_mutex_t      config_mux = PTHREAD_MUTEX_INITIALIZER;

static void meter_timer_cb(lv_timer_t *t);

void mic_init() {
    settings_mic_validate(&options->audio.mic);

    eq_config_t config = settings_mic_eq_config(&options->audio.mic);

    equalizer = eq_create(&config);

    /* liquid 1.6.0 bandwidth is relative to INPUT rate even when decimating.
     * 5 kHz cutoff, m=64, Kaiser 80 dB: transition finishes before 6.4 kHz.
     * Override the library's energy normalization to preserve amplitude. */

    float bw = 5000.0f / AUDIO_CAPTURE_RATE;

    resamp = rresamp_rrrf_create_kaiser(INTER, DECIM, 64, bw, 80);

    if (resamp) 
        rresamp_rrrf_set_scale(resamp, 2 * bw);

    record_resamp = rresamp_rrrf_create_kaiser(DECIM, INTER, 16, 0.45f, 80);

    if (record_resamp) 
        rresamp_rrrf_set_scale(record_resamp, 0.9f);

    agc = agc_create(
        AGC_FAST,               /* mode */
        DAC_RATE,               /* sample rate */
        0.001f,                 /* tau_attack */
        0.250f,                 /* tau_decay */
        4,                      /* n_tau */
        4.0f,                   /* max_gain */
        1.5f,                   /* var_gain */
        4.0f,                   /* fixed_gain */
        1.0f,                   /* max_input */
        1.0f,                   /* out_target */
        0.250f,                 /* tau_fast_backaverage */
        0.005f,                 /* tau_fast_decay */
        5.0f,                   /* pop_ratio */
        1,                      /* hang_enable */
        0.500f,                 /* tau_hang_backmult */
        0.250f,                 /* hangtime */
        0.250f,                 /* hang_thresh */
        0.100f                  /* tau_hang_decay */
    );

    if (!equalizer || !resamp || !record_resamp || !agc) {
        LV_LOG_ERROR("Cannot initialize microphone DSP");
    }

    lv_timer_create(meter_timer_cb, 1000 / 10, NULL);
}

void mic_update_filter() {
    pthread_mutex_lock(&config_mux);
    config_update = get_time() + 500;
    pthread_mutex_unlock(&config_mux);
}

void mic_update_equalizer() {
    mic_update_filter(); 
}

size_t mic_modulate(float complex *data, size_t max_size, radio_mode_t mode) {
    float   block[INTER];
    size_t  n = max_size < INTER ? max_size : INTER;

    pthread_mutex_lock(&output_mux);

    if (!on_air)
        n = 0;

    if (n > output_count)
        n = output_count;

    for (size_t i = 0; i < n; i++)
        block[i] = output[(output_read + i) % OUT_SIZE];

    output_read = (output_read + n) % OUT_SIZE;
    output_count -= n;

    pthread_mutex_unlock(&output_mux);

    for (size_t i = 0; i < n; i++)
        data[i] = dsp_modulate(block[i], mode);

    return n;
}

void mic_on_air(bool on) {
    pthread_mutex_lock(&output_mux);

    if (on_air != on) {
        on_air = on;
        stream_epoch++;
        output_read = output_count = 0;
    }

    pthread_mutex_unlock(&output_mux);
}

void mic_enabled(bool on) {
    enabled = on;
    stream_epoch++;
    brass_msg_send(MSG_MIC, &enabled);
}

static void meter_timer_cb(lv_timer_t *t) {
    pthread_mutex_lock(&meter_mux);

    float peak = meter_count ? meter_sum / meter_count : 0;

    meter_count = 0;
    meter_sum = 0;

    pthread_mutex_unlock(&meter_mux);

    lpf(&meter_avr, peak, 0.2f);
    brass_msg_send(MSG_MIC_METER, &meter_avr);
}

static void reset_stream(void) {
    input_fill = 0;
    rresamp_rrrf_reset(resamp);
    rresamp_rrrf_reset(record_resamp);
    eq_reset(equalizer);
    agc_flush(agc);

    pthread_mutex_lock(&output_mux);
    output_read = output_count = 0;
    pthread_mutex_unlock(&output_mux);
}

static void put_output(const float *block, size_t count, unsigned epoch) {
    pthread_mutex_lock(&output_mux);

    if (on_air && stream_epoch == epoch) {
        /* Capture cannot wait for a stalled transmitter. Drop oldest samples
         * on overrun so latency stays bounded; ordinary operation loses none. */
        if (count > OUT_SIZE - output_count) {
            size_t discard = count - (OUT_SIZE - output_count);

            output_read = (output_read + discard) % OUT_SIZE;
            output_count -= discard;
        }

        for (size_t i = 0; i < count; i++)
            output[(output_read + output_count + i) % OUT_SIZE] = block[i];

        output_count += count;
    }

    pthread_mutex_unlock(&output_mux);
}

void mic_put_audio_samples(size_t nsamples, int16_t *samples) {
    if (!equalizer || !resamp || !record_resamp || !agc) 
        return;

    bool        recording = dialog_msg_voice_get_state() == MSG_VOICE_RECORD;
    bool        active = on_air || recording;
    unsigned    epoch = stream_epoch;

    if (epoch != audio_epoch || active != was_active || recording != was_recording) {
        reset_stream();
        audio_epoch = epoch;
        was_active = active;
        was_recording = recording;
    }

    float peak = 0;

    pthread_mutex_lock(&config_mux);

    if (active && config_update) {
        if (get_time() > config_update) {
            config_update = 0;

            eq_config_t config = settings_mic_eq_config(&options->audio.mic);

            if (!eq_update(equalizer, &config)) 
               LV_LOG_WARN("Invalid microphone filter/EQ update");
        }
    }

    pthread_mutex_unlock(&config_mux);

    if (active) {
        for (size_t i = 0; i < nsamples; i++) {
            input[input_fill++] = samples ? samples[i] / 32768.0f : 0;

            if (input_fill != DECIM) 
                continue;

            float converted[INTER], filtered[EQ_HOP_SIZE];

            rresamp_rrrf_execute(resamp, input, converted);
            input_fill = 0;

            size_t consumed;

            size_t n = eq_process(equalizer, converted, INTER, filtered, EQ_HOP_SIZE, &consumed);

            for (size_t j = 0; j < n; j++) {
                filtered[j] = agc_apply(agc, filtered[j]);
                if (fabsf(filtered[j]) > peak) peak = fabsf(filtered[j]);
            }

            if (recording) {
                /* EQ outputs exactly 256 samples for every second 128-sample
                 * resampler block. Encode only 44.1-kHz PCM, including MP3. */
                for (size_t j = 0; j < n; j += INTER) {
                    float recorded[DECIM];

                    rresamp_rrrf_execute(record_resamp, filtered + j, recorded);
                    dialog_msg_voice_put_audio_samples(recorded, DECIM);
                }
            } else if (n) {
                put_output(filtered, n, epoch);
            }
        }
    }

    pthread_mutex_lock(&meter_mux);
    meter_sum += peak;
    meter_count++;
    pthread_mutex_unlock(&meter_mux);
}
