/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <math.h>
#include <pthread.h>
#include "lvgl/lvgl.h"
#include "rtty.h"
#include "audio.h"
#include "pannel.h"
#include "util.h"
#include "fpga/adc.h"
#include "main.h"
#include "main_screen.h"
#include "settings/options.h"

#define SYMBOL_OVER         8
#define SYMBOL_FACTOR       2
#define SYMBOL_LEN          (SYMBOL_OVER * SYMBOL_FACTOR)

#define RTTY_SYMBOL_CODE    (0b11011)
#define RTTY_LETTER_CODE    (0b11111)

typedef enum {
    RX_STATE_IDLE,
    RX_STATE_START,
    RX_STATE_DATA,
    RX_STATE_STOP
} rx_state_t;

static pthread_mutex_t  mux;
static pthread_cond_t   cond;

static fskdem           demod = NULL;

static nco_crcf         nco = NULL;
static float complex    *nco_buf = NULL;

static uint16_t         symbol_samples;
static uint16_t         symbol_over;

static cbuffercf        rx_buf;
static complex float    *rx_window = NULL;
static uint8_t          rx_symbol[SYMBOL_LEN];
static float            rx_symbol_pwr[SYMBOL_LEN];
static uint8_t          rx_symbol_cur = 0;
static rx_state_t       rx_state = RX_STATE_IDLE;
static uint8_t          rx_counter = 0;
static uint8_t          rx_bitcntr = 0;
static uint8_t          rx_data = 0;
static bool             rx_letter = true;

static bool             ready = false;
static bool             update = false;
static rtty_state_t     state = RTTY_OFF;

static void * rtty_thread(void *arg);

static const char rtty_letters[32] = {
    '\0',   'E',    '\n',   'A',    ' ',    'S',    'I',    'U',
    '\0',   'D',    'R',    'J',    'N',    'F',    'C',    'K',
    'T',    'Z',    'L',    'W',    'H',    'Y',    'P',    'Q',
    'O',    'B',    'G',    ' ',    'M',    'X',    'V',    ' '
};

static const char rtty_symbols[32] = {
    '\0',   '3',    '\n',   '-',    ' ',    '\0',   '8',    '7',
    '\0',   '$',    '4',    '\'',   ',',    '!',    ':',    '(',
    '5',    '"',    ')',    '2',    '#',    '6',    '0',    '1',
    '9',    '?',    '&',    ' ',    '.',    '/',    ';',    ' '
};

static void update_nco() {
    float radians = 2.0f * (float) M_PI * (float) options->rtty.center / (float) ADC_RATE;

    nco_crcf_set_phase(nco, 0.0f);
    nco_crcf_set_frequency(nco, radians);
}

static void init() {
    symbol_samples = (float) ADC_RATE / (float) options->rtty.rate / (float) SYMBOL_FACTOR + 0.5f;

    symbol_over = symbol_samples / SYMBOL_OVER;

    nco = nco_crcf_create(LIQUID_NCO);
    nco_buf = (float complex*) malloc(symbol_samples * sizeof(float complex));
    update_nco();

    /* RX */

    demod = fskdem_create(1, symbol_samples, (float) options->rtty.shift / (float) ADC_RATE / 2.0f);
    rx_buf = cbuffercf_create(symbol_samples * 50);

    rx_window = malloc(symbol_samples * sizeof(complex float));

    for (uint16_t i = 0; i < symbol_samples; i++)
        rx_window[i] = liquid_hann(i, symbol_samples);

    ready = true;
}

static void done() {
    ready = false;

    nco_crcf_destroy(nco);
    free(nco_buf);

    fskdem_destroy(demod);
    cbuffercf_destroy(rx_buf);
    free(rx_window);
}

void rtty_init() {
    pthread_mutex_init(&mux, NULL);
    pthread_cond_init(&cond, NULL);

    init();

    pthread_t thread;

    pthread_create(&thread, NULL, rtty_thread, NULL);
    pthread_detach(thread);
}

static char baudot_decoder(uint8_t c) {
    if (c == RTTY_SYMBOL_CODE) {
        rx_letter = false;
        return 0;
    }

    if (c == RTTY_LETTER_CODE) {
        rx_letter = true;
        return 0;
    }

    return rx_letter ? rtty_letters[c] : rtty_symbols[c];
}

static bool is_mark_space(uint8_t *correction) {
    uint16_t res = 0;

    if (rx_symbol[0] && !rx_symbol[SYMBOL_LEN-1]) {
        for (int i = 0; i < SYMBOL_LEN; i++)
            res += rx_symbol[i];

        if (abs(SYMBOL_LEN/2 - res) < 1) {
            *correction = res;
            return true;
        }
    }
    return false;
}

static bool is_mark() {
    return rx_symbol[SYMBOL_LEN / 2];
}

static void add_symbol(float pwr) {
    for (uint8_t i = 1; i < SYMBOL_LEN; i++) {
        rx_symbol[i - 1] = rx_symbol[i];
        rx_symbol_pwr[i - 1] = rx_symbol_pwr[i];
    }

    rx_symbol_pwr[SYMBOL_LEN - 1] = pwr;

    float   p_avr = 0.0f;
    uint8_t p_num = SYMBOL_LEN / 2;

    for (uint8_t i = SYMBOL_LEN - p_num; i < SYMBOL_LEN; i++)
        p_avr += rx_symbol_pwr[i];

    p_avr /= (float) p_num;

    if (rx_symbol_cur == 0) {
        if (p_avr > options->rtty.snr) {
            rx_symbol_cur = 1;
        }
    } else {
        if (p_avr < -options->rtty.snr) {
            rx_symbol_cur = 0;
        }
    }

//    LV_LOG_INFO("%5.1f %i", p_avr, rx_symbol_cur);

    rx_symbol[SYMBOL_LEN - 1] = rx_symbol_cur;

    uint8_t correction;

    switch (rx_state) {
        case RX_STATE_IDLE:
            if (is_mark_space(&correction)) {
                rx_state = RX_STATE_START;
                rx_counter = correction;
            }
            break;

        case RX_STATE_START:
            if (--rx_counter == 0) {
                if (!is_mark()) {
                    rx_state = RX_STATE_DATA;
                    rx_counter = SYMBOL_LEN;
                    rx_bitcntr = 0;
                    rx_data = 0;
                } else {
                    rx_state = RX_STATE_IDLE;
                }
            }
            break;

        case RX_STATE_DATA:
            if (--rx_counter == 0) {
                rx_data |= is_mark() << rx_bitcntr++;
                rx_counter = SYMBOL_LEN;
            }

            if (rx_bitcntr == options->rtty.bits)
                rx_state = RX_STATE_STOP;
            break;

        case RX_STATE_STOP:
            if (--rx_counter == 0) {
                if (is_mark()) {
                    char c = baudot_decoder(rx_data);

                    if (c) {
                        char str[2] = { c, 0 };

                        brass_lv_lock();
                        pannel_add_text(str);
                        brass_lv_unlock();
                    }
                }
                rx_state = RX_STATE_IDLE;
            }
            break;
    }
}

void rtty_put_audio_samples(float complex *samples, size_t n) {
    if (!ready) {
        return;
    }

    pthread_mutex_lock(&mux);
    cbuffercf_write(rx_buf, samples, n);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mux);
}

static void * rtty_thread(void *arg) {
    while (true) {
        pthread_mutex_lock(&mux);
        pthread_cond_wait(&cond, &mux);

        if (update) {
            done();
            init();
            update = false;
        }

        pthread_mutex_unlock(&mux);

        while (cbuffercf_size(rx_buf) > symbol_samples) {
            unsigned int    symbol;
            unsigned int    n;
            float complex   *buf;

            cbuffercf_read(rx_buf, symbol_samples, &buf, &n);
            nco_crcf_mix_block_down(nco, buf, nco_buf, n);

            for (uint16_t i = 0; i < n; i++)
                nco_buf[i] *= rx_window[i];

            fskdem_demodulate(demod, nco_buf);

            float f0 = fskdem_get_symbol_energy(demod, 0, 1);
            float f1 = fskdem_get_symbol_energy(demod, 1, 1);

            float pwr0 = 10.0f * log10f(f0);
            float pwr1 = 10.0f * log10f(f1);

            float pwr = pwr0 - pwr1;

            if (options->rtty.reverse) {
                pwr = -pwr;
            }

            add_symbol(pwr);
            cbuffercf_release(rx_buf, symbol_over);
        }
    }
}

void rtty_set_state(rtty_state_t x) {
    state = x;
}

rtty_state_t rtty_get_state() {
    return state;
}

float rtty_change_rate(int16_t df) {
    const float rates[] = { 45.0f, 45.45f, 50.0f, 56.0f, 75.0f, 100.0f, 110.0f, 150.0f };

    if (df == 0) {
        return options->rtty.rate;
    }

    for (int16_t i = 0; i < 8; i++)
        if (rates[i] == options->rtty.rate) {
            i = limit(i + df, 0, 7);
            options->rtty.rate = rates[i];

            break;
        }

    update = true;
    return options->rtty.rate;
}

uint16_t rtty_change_shift(int16_t df) {
    if (df == 0) {
        return options->rtty.shift;
    }

    switch (options->rtty.shift) {
        case 170:
            options->rtty.shift = df > 0 ? 425 : 170;
            break;

        case 425:
            options->rtty.shift = df > 0 ? 450 : 170;
            break;

        case 450:
            options->rtty.shift = df > 0 ? 850 : 425;
            break;

        case 850:
            options->rtty.shift = df > 0 ? 850 : 450;
            break;

        default:
            options->rtty.shift = 170;
            break;
    }

    update = true;
    main_screen_update_finder();

    return options->rtty.shift;
}

uint16_t rtty_change_center(int16_t df) {
    if (df == 0) {
        return options->rtty.center;
    }

    options->rtty.center = limit(align_int(options->rtty.center + df * 5, 5), 400, 2000);

    pthread_mutex_lock(&mux);
    update_nco();
    pthread_mutex_unlock(&mux);
    main_screen_update_finder();

    return options->rtty.center;
}

bool rtty_change_reverse(int16_t df) {
    if (df == 0) {
        return options->rtty.reverse;
    }

    options->rtty.reverse = !options->rtty.reverse;

    return options->rtty.reverse;
}
