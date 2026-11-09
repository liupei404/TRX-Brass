/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <stdint.h>
#include <math.h>
#include <pthread.h>

#include "radio.h"
#include "msg.h"
#include "msgs.h"
#include "swr.h"
#include "util.h"
#include "settings/rf.h"

static float            a0, a1, a2, a3;
static uint64_t         offset_delay = 0;

static float            swr = 0;
static float            pwr = 0;

static float            peak_swr = 0;
static float            peak_pwr = 0;

static bool             meter_pending = false;
static pthread_mutex_t  meter_mux = PTHREAD_MUTEX_INITIALIZER;

static void meter_timer_cb(lv_timer_t *timer) {
    (void) timer;

    pthread_mutex_lock(&meter_mux);

    if (meter_pending && radio_get_state() != RADIO_RX) {
        brass_msg_send(MSG_SWR_METER, &peak_swr);
        brass_msg_send(MSG_PWR_METER, &peak_pwr);

        meter_pending = false;
        peak_swr = 0;
        peak_pwr = 0;
    }

    pthread_mutex_unlock(&meter_mux);
}

void swr_init() {
    double s[7] = { 0 };
    double t[4] = { 0 };

    for (size_t i = 0; i < rf->swr.count; i++) {
        double x = rf->swr.calibrate[i].adc;
        double y = rf->swr.calibrate[i].vpp;
        double xp = 1.0;

        for (int k = 0; k <= 6; k++) {
            s[k] += xp;
            xp *= x;
        }

        xp = 1.0;

        for (int k = 0; k <= 3; k++) {
            t[k] += y * xp;
            xp *= x;
        }
    }

    double a[4][5];

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++)
            a[row][col] = s[row + col];

        a[row][4] = t[row];
    }

    /*
     * Gaussian elimination with partial pivoting.
     */

    for (int col = 0; col < 4; col++) {
        int pivot = col;

        for (int row = col + 1; row < 4; row++) {
            if (fabs(a[row][col]) > fabs(a[pivot][col]))
                pivot = row;
        }

        if (fabs(a[pivot][col]) < 1e-12) {
            LV_LOG_ERROR("Ops!");
            return;
        }

        if (pivot != col) {
            for (int k = col; k < 5; ++k) {
                double tmp = a[col][k];

                a[col][k] = a[pivot][k];
                a[pivot][k] = tmp;
            }
        }

        double div = a[col][col];

        for (int k = col; k < 5; k++)
            a[col][k] /= div;

        for (int row = 0; row < 4; row++) {
            if (row == col)
                continue;

            double factor = a[row][col];

            for (int k = col; k < 5; k++)
                a[row][k] -= factor * a[col][k];
        }
    }

    a0 = (float)a[0][4];
    a1 = (float)a[1][4];
    a2 = (float)a[2][4];
    a3 = (float)a[3][4];

    lv_timer_create(meter_timer_cb, 1000 / 10, NULL);
}

void swr_update(int fwd, int rev) {
    uint64_t now = get_time();

    if (radio_get_state() == RADIO_RX) {
        if (now > offset_delay) {
            rf->swr.fwd_offset = fwd;
            rf->swr.rev_offset = rev;

            pthread_mutex_lock(&meter_mux);
            peak_swr = 0;
            peak_pwr = 0;
            pthread_mutex_unlock(&meter_mux);

            offset_delay = now + 100;
        }

        return;
    } else {
        offset_delay = now + 1000;
    }

    fwd -= rf->swr.fwd_offset;
    rev -= rf->swr.rev_offset;

    float v_fwd = a0 + a1 * fwd + a2 * fwd * fwd + a3 * fwd * fwd * fwd;
    float v_rev = a0 + a1 * rev + a2 * rev * rev + a3 * rev * rev * rev;

    float g = v_rev / v_fwd;
    float v_rms = v_fwd / 2.82842712474619f;

    pthread_mutex_lock(&meter_mux);
    swr = (1.0f + g) / (1.0f - g);
    pwr = (v_rms * v_rms) / 50.0f;

    if (pwr > peak_pwr) {
        peak_pwr = pwr;
    }

    if (swr > peak_swr) {
        peak_swr = swr;
    }

    meter_pending = true;
    pthread_mutex_unlock(&meter_mux);
}
