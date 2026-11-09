/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iio.h>

#include "lvgl/lvgl.h"
#include "iio.h"
#include "iio_adc.h"
#include "src/hkey.h"
#include "src/swr.h"

static struct iio_channel  *vref = NULL;
static struct iio_channel  *hkeys_x = NULL;
static struct iio_channel  *hkeys_y = NULL;
static struct iio_channel  *pwr_rev = NULL;
static struct iio_channel  *pwr_fwd = NULL;
static struct iio_context  *ctx = NULL;
static pthread_t           keys_thread;
static bool                keys_started;
static atomic_bool         keys_running;
static pthread_mutex_t     vref_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Give buffered power measurements time between raw keyboard polls. */
#define HKEY_POLL_US 20000

static void * iio_thread(void *arg) {
    (void) arg;
    unsigned int failures = 0;
    while (atomic_load(&keys_running)) {
        long long x, y;

        int rx = iio_channel_attr_read_longlong(hkeys_x, "raw", &x);
        int ry = iio_channel_attr_read_longlong(hkeys_y, "raw", &y);

        if (rx >= 0 && ry >= 0) {
            hkey_put(x, y);
            failures = 0;
        } else if (failures++ % 250 == 0) {
            int error = rx < 0 ? rx : ry;
            fprintf(stderr, "ADC keys: raw read: %s (%d)\n", strerror(-error), error);
        }

        usleep(HKEY_POLL_US);
    }

    return NULL;
}

void iio_init() {
    struct iio_device   *dev = NULL;
    static bool cleanup_registered;

    if (ctx)
        return;

    if (!cleanup_registered) {
        if (atexit(iio_shutdown)) {
            LV_LOG_ERROR("Register IIO cleanup");
            return;
        }
        cleanup_registered = true;
    }

    ctx = iio_create_local_context();

    if (ctx == NULL) {
        LV_LOG_ERROR("Create context");
        return;
    }
    if (iio_context_set_timeout(ctx, 1000) < 0) {
        LV_LOG_ERROR("Set IIO timeout");
        iio_shutdown();
        return;
    }

    /* * */

    dev = iio_context_find_device(ctx, "mcp4725");

    if (dev != NULL) {
        vref = iio_device_find_channel(dev, "voltage0", true);

        if (vref == NULL) {
            LV_LOG_ERROR("Find mcp4725 channel");
        }
    } else {
        LV_LOG_ERROR("Find mcp4725");
    }


    /* * */

    dev = iio_context_find_device(ctx, "ads1015");

    if (dev != NULL) {
        hkeys_x = iio_device_find_channel(dev, "voltage0", false);

        if (hkeys_x == NULL) {
            LV_LOG_ERROR("Find ads1015 channel");
        }

        hkeys_y = iio_device_find_channel(dev, "voltage1", false);

        if (hkeys_y == NULL) {
            LV_LOG_ERROR("Find ads1015 channel");
        }

        pwr_fwd = iio_device_find_channel(dev, "voltage2", false);

        if (pwr_fwd == NULL) {
            LV_LOG_ERROR("Find ads1015 channel");
        }

        pwr_rev = iio_device_find_channel(dev, "voltage3", false);

        if (pwr_rev == NULL) {
            LV_LOG_ERROR("Find ads1015 channel");
        }
    } else {
        LV_LOG_ERROR("Find ads1015");
    }

    /* Independent workers: raw keys and buffered forward/reflected power. */

    if (hkeys_x && hkeys_y) {
        /* Slow raw conversions also stall buffered power scans on this ADC. */

        if (iio_adc_set_sampling_frequency(hkeys_x) < 0 ||
            iio_adc_set_sampling_frequency(hkeys_y) < 0)
            LV_LOG_ERROR("Set ADC keyboard sampling frequency");

        atomic_store(&keys_running, true);

        int ret = pthread_create(&keys_thread, NULL, iio_thread, NULL);

        keys_started = ret == 0;

        if (ret) {
            atomic_store(&keys_running, false);
            LV_LOG_ERROR("Create IIO keys thread: %s", strerror(ret));
        }
    }

    if (dev && pwr_fwd && pwr_rev && iio_adc_start(dev, pwr_fwd, pwr_rev, swr_update) < 0)
        LV_LOG_ERROR("Start buffered SWR/PWR worker");
}

void iio_shutdown(void) {
    atomic_store(&keys_running, false);
    iio_adc_stop();

    if (keys_started) {
        pthread_join(keys_thread, NULL);
        keys_started = false;
    }

    pthread_mutex_lock(&vref_mutex);

    vref = hkeys_x = hkeys_y = pwr_fwd = pwr_rev = NULL;

    if (ctx) {
        iio_context_destroy(ctx);
        ctx = NULL;
    }

    pthread_mutex_unlock(&vref_mutex);
}

void iio_set_vref(uint16_t data) {
    pthread_mutex_lock(&vref_mutex);

    if (vref == NULL) {
        pthread_mutex_unlock(&vref_mutex);
        return;
    }

    if (iio_channel_attr_write_longlong(vref, "raw", data) < 0) {
        LV_LOG_ERROR("Write to VRef");
    }

    pthread_mutex_unlock(&vref_mutex);
}
