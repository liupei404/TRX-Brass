/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include "eq.h"

#include <fftw3.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define DESIGN_SIZE 8192
#define PI 3.14159265358979323846

struct eq {
    double          *time;
    fftw_complex    *spectrum;
    fftw_plan       forward, inverse;
    double          history[EQ_HOP_SIZE];
    float           incoming[EQ_HOP_SIZE], ready[EQ_HOP_SIZE];
    size_t          fill, read, available;
    fftw_complex    response[EQ_TAPS], pending[EQ_TAPS];
    pthread_mutex_t update_lock;
    bool            updated;
    double          sample_rate;
};

bool eq_valid_config(const eq_config_t *c) {
    if (!c || !isfinite(c->sample_rate) || c->sample_rate <= 0 ||
        !isfinite(c->low) || !isfinite(c->high) || !isfinite(c->transition) ||
        c->low < 0 || c->high >= c->sample_rate / 2 || c->transition < 50 ||
        c->low + 2 * c->transition > c->high ||
        c->count < 1 || c->count > EQ_MAX_POINTS) return false;

    for (size_t i = 0; i < c->count; i++) {
        if (c->points[i].freq < 0 || c->points[i].freq >= c->sample_rate / 2 ||
            c->points[i].gain < -24 || c->points[i].gain > 12 ||
            (i && c->points[i].freq <= c->points[i-1].freq)) return false;
    }

    return true;
}

double eq_target(const eq_config_t *c, double f) {
    if (f <= c->low || f >= c->high) return 0;

    double band = 1;

    if (f < c->low + c->transition)
        band *= 0.5 - 0.5 * cos(PI * (f - c->low) / c->transition);

    if (f > c->high - c->transition)
        band *= 0.5 - 0.5 * cos(PI * (c->high - f) / c->transition);

    double gain = c->points[0].gain;

    for (size_t i = 1; i < c->count; i++) {
        if (f <= c->points[i-1].freq) break;

        if (f < c->points[i].freq) {
            double t = (f - c->points[i-1].freq) /
                       (c->points[i].freq - c->points[i-1].freq);
            gain = c->points[i-1].gain + t * (c->points[i].gain - c->points[i-1].gain);
            break;
        }

        gain = c->points[i].gain;
    }

    return band * pow(10, gain / 20);
}

bool eq_design(const eq_config_t *c, double taps[EQ_TAPS]) {
    if (!eq_valid_config(c)) return false;

    double mask[DESIGN_SIZE / 2 + 1];

    for (int k = 0; k <= DESIGN_SIZE / 2; k++)
        mask[k] = eq_target(c, k * c->sample_rate / DESIGN_SIZE);

    double sum = 0, window_sum = 0;

    /* The central 257 samples of a dense inverse real DFT. Computing only
     * these samples avoids creating FFTW plans during settings updates. */

    for (int n = 0; n <= EQ_HOP_SIZE / 2; n++) {
        double value = mask[0] + mask[DESIGN_SIZE / 2] * (n % 2 ? -1 : 1);

        for (int k = 1; k < DESIGN_SIZE / 2; k++)
            value += 2 * mask[k] * cos(2 * PI * k * n / DESIGN_SIZE);

        double window = 0.54 + 0.46 * cos(PI * n / (EQ_HOP_SIZE / 2));

        value *= window / DESIGN_SIZE;
        taps[EQ_HOP_SIZE / 2 - n] = taps[EQ_HOP_SIZE / 2 + n] = value;
        sum += value * (n ? 2 : 1);
        window_sum += window * (n ? 2 : 1);
    }

    /* Remove residual DC with a symmetric, window-shaped correction. Unlike
     * clearing one spectrum bin this preserves finite support and linear phase. */

    for (int i = 0; i < EQ_TAPS; i++) {
        double window = 0.54 - 0.46 * cos(2 * PI * i / (EQ_TAPS - 1));

        taps[i] -= sum * window / window_sum;

        if (!isfinite(taps[i])) return false;
    }
    return true;
}

bool eq_update(eq_t *q, const eq_config_t *c) {
    double taps[EQ_TAPS];

    fftw_complex h[EQ_TAPS];

    if (!q || !c || c->sample_rate != q->sample_rate || !eq_design(c, taps)) return false;

    for (int k = 0; k < EQ_TAPS; k++) {
        h[k][0] = h[k][1] = 0;

        for (int n = 0; n < EQ_TAPS; n++) {
            double phase = 2 * PI * k * n / EQ_FFT_SIZE;

            h[k][0] += taps[n] * cos(phase);
            h[k][1] -= taps[n] * sin(phase);
        }
    }

    pthread_mutex_lock(&q->update_lock);
    memcpy(q->pending, h, sizeof(h));
    q->updated = true;
    pthread_mutex_unlock(&q->update_lock);

    return true;
}

eq_t *eq_create(const eq_config_t *c) {
    if (!eq_valid_config(c)) return NULL;

    eq_t *q = calloc(1, sizeof(*q));

    if (!q) return NULL;

    if (pthread_mutex_init(&q->update_lock, NULL)) {
        free(q); return NULL;
    }

    q->sample_rate = c->sample_rate;
    q->time = fftw_alloc_real(EQ_FFT_SIZE);
    q->spectrum = fftw_alloc_complex(EQ_TAPS);

    if (!q->time || !q->spectrum) {
        eq_destroy(q); 
        return NULL;
    }

    q->forward = fftw_plan_dft_r2c_1d(EQ_FFT_SIZE, q->time, q->spectrum, FFTW_ESTIMATE);
    q->inverse = fftw_plan_dft_c2r_1d(EQ_FFT_SIZE, q->spectrum, q->time, FFTW_ESTIMATE);

    if (!q->forward || !q->inverse || !eq_update(q, c)) {
        eq_destroy(q); 
        return NULL;
    }

    memcpy(q->response, q->pending, sizeof(q->response));
    q->updated = false;

    return q;
}

void eq_destroy(eq_t *q) {
    if (!q) return;

    if (q->forward)
        fftw_destroy_plan(q->forward);

    if (q->inverse)
        fftw_destroy_plan(q->inverse);

    fftw_free(q->time);
    fftw_free(q->spectrum);

    pthread_mutex_destroy(&q->update_lock);
    free(q);
}

void eq_reset(eq_t *q) {
    memset(q->history, 0, sizeof(q->history));
    q->fill = q->read = q->available = 0;
}

static void process_block(eq_t *q) {
    if (pthread_mutex_trylock(&q->update_lock) == 0) {
        if (q->updated) {
            memcpy(q->response, q->pending, sizeof(q->response));
            q->updated = false;
        }
        pthread_mutex_unlock(&q->update_lock);
    }

    memcpy(q->time, q->history, sizeof(q->history));

    for (int i = 0; i < EQ_HOP_SIZE; i++)
        q->time[EQ_HOP_SIZE + i] = q->history[i] = q->incoming[i];

    fftw_execute(q->forward);

    for (int k = 0; k < EQ_TAPS; k++) {
        double re = q->spectrum[k][0], im = q->spectrum[k][1];

        q->spectrum[k][0] = re * q->response[k][0] - im * q->response[k][1];
        q->spectrum[k][1] = re * q->response[k][1] + im * q->response[k][0];
    }

    fftw_execute(q->inverse);

    for (int i = 0; i < EQ_HOP_SIZE; i++)
        q->ready[i] = q->time[EQ_HOP_SIZE + i] / EQ_FFT_SIZE;

    q->fill = q->read = 0;
    q->available = EQ_HOP_SIZE;
}

size_t eq_process(eq_t *q, const float *in, size_t count, float *out, size_t capacity, size_t *consumed) {
    size_t used = 0, written = 0;

    while (written < capacity) {
        if (q->available) {
            size_t n = q->available < capacity - written ? q->available : capacity - written;

            memcpy(out + written, q->ready + q->read, n * sizeof(float));
            q->read += n;
            q->available -= n;
            written += n;
        } else if (used < count) {
            size_t n = EQ_HOP_SIZE - q->fill;

            if (n > count - used) 
                n = count - used;

            memcpy(q->incoming + q->fill, in + used, n * sizeof(float));
            q->fill += n;
            used += n;

            if (q->fill == EQ_HOP_SIZE)
                process_block(q);
        } else break;
    }

    *consumed = used;
    return written;
}
