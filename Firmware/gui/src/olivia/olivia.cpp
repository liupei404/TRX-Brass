/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "olivia.h"

extern "C" {

#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include "lvgl/lvgl.h"

#include "src/main.h"
#include "src/main_screen.h"
#include "src/pannel.h"
#include "src/msg.h"
#include "src/fpga/adc.h"
#include "src/settings/options.h"
#include "src/settings/modes.h"
#include "src/util.h"

extern void dsp_set_filter(filter_t *filter);

}

#define DECIM 128
#define INTER 80

#include "mfsk_parameters.h"
#include "mfsk_receiver.h"

static pthread_mutex_t  mux;
static pthread_cond_t   cond;

static cbufferf         in_buf;
static rresamp_rrrf     resamp;
static float            resamp_buf[INTER];

static MFSK_Parameters  parameters;
static pthread_mutex_t  parameters_mux;
static bool             parameters_update = false;
static MFSK_Receiver    rx;
static cbufferf         rx_buf;
static float            *rx_window = NULL;

static void * olivia_thread(void *arg);

static void init() {
    parameters.SetTones(options->olivia.tones);
    parameters.SetBandWidth(options->olivia.band_width);
    parameters.SetBandLower(options->olivia.band_lower);
    parameters.SetSyncMargin(options->olivia.sync_margin);
    parameters.SetSyncInteg(options->olivia.sync_integ);
    parameters.SetSyncThreshold(options->olivia.sync_threshold);

    parameters.Preset();
    rx.Preset(&parameters);

    size_t window = rx.WindowLen();

    rx_buf = cbufferf_create(window * 2);
    rx_window = (float *) malloc(window * sizeof(float));

    float arg = 10.0f;

    for (size_t i = 0; i < window; i++)
        rx_window[i] = liquid_windowf(LIQUID_WINDOW_HAMMING, i, window, arg);
}

static void done() {
    cbufferf_destroy(rx_buf);
    free(rx_window);
}

void olivia_init() {
    pthread_mutex_init(&mux, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_mutex_init(&parameters_mux, NULL);

    resamp = rresamp_rrrf_create_default(INTER, DECIM);     /* OLIVIA_RATE <- ADC_RATE */
    in_buf = cbufferf_create(ADC_SAMPLES * 10);

    init();

    pthread_t thread;

    pthread_create(&thread, NULL, olivia_thread, NULL);
    pthread_detach(thread);
}

static void * olivia_thread(void *arg) {
    unsigned int n;
    float *buf;

    while (true) {
        pthread_mutex_lock(&mux);
        pthread_cond_wait(&cond, &mux);
        pthread_mutex_unlock(&mux);

        pthread_mutex_lock(&parameters_mux);

        if (parameters_update) {
            done();
            init();
            parameters_update = false;
        }

        pthread_mutex_unlock(&parameters_mux);

        while (cbufferf_size(rx_buf) >= rx.WindowLen()) {
            cbufferf_read(rx_buf, rx.WindowLen(), &buf, &n);

            for (size_t i = 0; i < rx.WindowLen(); i++)
                buf[i] *= rx_window[i];

            rx.ProcessInputBuffer(buf);
            cbufferf_release(rx_buf, n);
        }

        uint8_t c;
        char    str[32] = "";
        char    *str_ptr = str;

        while (rx.GetChar(c) > 0) {
            if (c == '\n') {
                c = ' ';
            }

            if (isprint(c) || c == '\n') {
                str_ptr[0] = c;
                str_ptr[1] = 0;

                str_ptr++;
            }
        }

        if (strlen(str) > 0) {
            pannel_add_text(str);
        }
    }
}

int olivia_put_audio_samples(int16_t *buf, size_t samples) {
    pthread_mutex_lock(&mux);

    for (size_t i = 0; i < samples; i++) {
        float x = buf[i];

        cbufferf_push(in_buf, x);
    }

    while (cbufferf_size(in_buf) > DECIM) {
        unsigned int n;
        float *buf;

        cbufferf_read(in_buf, DECIM, &buf, &n);
        rresamp_rrrf_execute(resamp, buf, resamp_buf);
        cbufferf_release(in_buf, n);

        pthread_mutex_lock(&parameters_mux);
        cbufferf_write(rx_buf, resamp_buf, INTER);
        pthread_mutex_unlock(&parameters_mux);
    }

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mux);

    return samples;
}

void olivia_info_cb(lv_event_t * e) {
    msg_set_text_fmt("Offset: %.1f, SNR: %.1f", rx.FrequencyOffset(), rx.InputSNRdB());
}

uint16_t olivia_change_tones(int16_t df) {
    if (df != 0) {
        int16_t bits = limit(Log2(options->olivia.tones) + df, 1, 8);
        options->olivia.tones = Exp2(bits);

        pthread_mutex_lock(&parameters_mux);
        parameters_update = true;
        pthread_mutex_unlock(&parameters_mux);
    }

    return options->olivia.tones;
}

uint16_t olivia_change_width(int16_t df) {
    if (df != 0) {
        int16_t x = options->olivia.band_width;

        switch (x) {
            case 125:
                x = df < 0 ? 125 : 250;
                break;

            case 250:
                x = df < 0 ? 125 : 500;
                break;

            case 500:
                x = df < 0 ? 250 : 1000;
                break;

            case 1000:
                x = df < 0 ? 500 : 2000;
                break;

            case 2000:
                x = df < 0 ? 1000 : 2000;
                break;

            default:
                x = 1000;
                break;
        }

        options->olivia.band_width = x;

        pthread_mutex_lock(&parameters_mux);
        parameters_update = true;
        pthread_mutex_unlock(&parameters_mux);

        main_screen_update_finder();
    }

    return options->olivia.band_width;
}

void olivia_preset(uint16_t x) {
    switch (x) {
        case 0:
            options->olivia.tones = 8;
            options->olivia.band_width = 250;
            break;

        case 1:
            options->olivia.tones = 8;
            options->olivia.band_width = 500;
            break;

        case 2:
            options->olivia.tones = 16;
            options->olivia.band_width = 500;
            break;

        case 3:
            options->olivia.tones = 32;
            options->olivia.band_width = 1000;
            break;
    }

    op_mode->filter.low = options->olivia.band_lower - 50;
    op_mode->filter.high = options->olivia.band_lower + options->olivia.band_width + 50;
    op_mode->filter.transition = 50;

    dsp_set_filter(&op_mode->filter);

    pthread_mutex_lock(&parameters_mux);
    parameters_update = true;
    pthread_mutex_unlock(&parameters_mux);

    main_screen_update_finder();
}
