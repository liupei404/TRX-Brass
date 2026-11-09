/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "power_meter.h"
#include "src/radio.h"

#define POWER_PATH  "/sys/class/hwmon/hwmon1/"

static int fd_pwr_v;
static int fd_pwr_a;
static int fd_pa_v;
static int fd_pa_a;

static int read_hwmon(int fd, int *value) {
    char buf[32];

    if (lseek(fd, 0, SEEK_SET) < 0)
        return -1;

    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    if (n <= 0)
        return -1;

    buf[n] = '\0';

    *value = atoi(buf);

    return 0;
}

static void * pa_thread(void *arg) {
    int mv = 0, ma = 0;

    while (true) {
        read_hwmon(fd_pa_v, &mv);
        read_hwmon(fd_pa_a, &ma);

        radio_update_pa_meter(abs(mv), abs(ma));
        usleep(1000);
    }
}

static void * power_thread(void *arg) {
    while (true) {
        usleep(10000);
    }
}

void power_meter_init() {
    fd_pa_v = open(POWER_PATH "in1_input", O_RDONLY);

    if (fd_pa_v <= 0) {
        LV_LOG_ERROR("Find PA voltage");
        return;
    }

    fd_pa_a = open(POWER_PATH "curr1_input", O_RDONLY);

    if (fd_pa_a <= 0) {
        LV_LOG_ERROR("Find PA current");
        return;
    }

    fd_pwr_v = open(POWER_PATH "in2_input", O_RDONLY);

    if (fd_pwr_v <= 0) {
        LV_LOG_ERROR("Find power voltage");
        return;
    }

    fd_pwr_a = open(POWER_PATH "curr2_input", O_RDONLY);

    if (fd_pwr_a <= 0) {
        LV_LOG_ERROR("Find PA current");
        return;
    }

    /* * */

    pthread_t thread;

    pthread_create(&thread, NULL, pa_thread, NULL);
    pthread_detach(thread);

    pthread_create(&thread, NULL, power_thread, NULL);
    pthread_detach(thread);
}
