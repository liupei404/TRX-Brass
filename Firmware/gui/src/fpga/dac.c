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
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <math.h>

#include "lvgl/lvgl.h"
#include "src/dsp.h"
#include "src/hw/gpio.h"
#include "dac.h"

static int              fd;
static float complex    samples[DAC_SAMPLES];

static void * dac_thread(void *arg) {
    uint32_t write_errors = 0;

    while (true) {
        size_t size = dsp_dac(samples, DAC_SAMPLES);

        if (size > 0) {
            size_t  bytes = size * sizeof(float complex);
            ssize_t res = write(fd, samples, bytes);

            if (res < 0) {
                if (errno == EINTR) {
                    continue;
                }

                LV_LOG_ERROR("DAC write failed: %s", strerror(errno));
                write_errors++;
            } else if ((size_t) res != bytes) {
                LV_LOG_WARN("DAC short write: %zd of %zu bytes", res, bytes);
                write_errors++;
            } else {
                write_errors = 0;
            }

            /* 只在首次达到阈值时上报，避免刷屏 */
            if (write_errors == 10) {
                LV_LOG_ERROR("DAC FIFO: 10 consecutive write errors");
            }
        } else {
            memset(samples, 0, sizeof(samples));
            write(fd, samples, sizeof(samples));
        }
    }
}

bool dac_init() {
    /* Stream */

    fd = open("/dev/axis_fifo_0x43c10000", O_WRONLY);

    if (fd < 1) {
        LV_LOG_ERROR("Unable to open DAC stream device file");
        return false;
    }

    pthread_t thread;

    pthread_create(&thread, NULL, dac_thread, NULL);
    pthread_detach(thread);

    return true;
}
