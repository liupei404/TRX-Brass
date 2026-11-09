/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>

#include "lvgl/lvgl.h"
#include "src/dsp.h"
#include "adc.h"
#include "control.h"

static int              fd;
static float complex    samples[ADC_SAMPLES];

static void * adc_thread(void *arg) {
    control_set_rx_rate(ADC_RATE);
    control_rx_enable();

    while (true) {
        int res = read(fd, samples, sizeof(samples));

        if (res > 0) {
            dsp_adc(samples, res / sizeof(float complex));
        }
    }
}

bool adc_init() {
    /* Stream */

    fd = open("/dev/axis_fifo_0x43c10000", O_RDONLY);

    if (fd < 1) {
        LV_LOG_ERROR("Unable to open ADC stream device file");
        return false;
    }

    pthread_t thread;

    pthread_create(&thread, NULL, adc_thread, NULL);
    pthread_detach(thread);

    return true;
}
