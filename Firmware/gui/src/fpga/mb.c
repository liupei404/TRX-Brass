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
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "src/dsp.h"
#include "mb.h"
#include "mb_data.h"
#include "control.h"

#define FW_DIR "/usr/share/brass/fw/"

static int              fd_i;
static int              fd_d;

static uint8_t          *bram_i;
static uint8_t          *bram_d;

static uint32_t         count = 0;

void mb_load(const char *filename, uint8_t *bram, bool swap) {
    int fd = open(filename, O_RDONLY);

    if (fd > 0) {
        uint8_t buf[4];

        while (1) {
            int res = read(fd, buf, sizeof(buf));

            if (res <= 0) {
                break;
            }

            if (swap) {
                *bram = buf[3]; bram++;
                *bram = buf[2]; bram++;
                *bram = buf[1]; bram++;
                *bram = buf[0]; bram++;
            } else {
                *bram = buf[0]; bram++;
                *bram = buf[1]; bram++;
                *bram = buf[2]; bram++;
                *bram = buf[3]; bram++;
            }
        }

        LV_LOG_INFO("%s loaded", filename);
        close(fd);
    } else {
        LV_LOG_ERROR("Can not open %s", filename);
    }
}

static void * mb_thread(void *arg) {
    mb_data_t   *data = (mb_data_t *) bram_d;

    while (true) {
        if (data->count != count) {
            dsp_fft(data->out);
            count = data->count;
        } else {
            usleep(5000);
        }
    }
}

bool mb_init() {
    /* I-BRAM */

    fd_i = open("/dev/uio1", O_RDWR | O_SYNC);

    if (fd_i < 1) {
        LV_LOG_ERROR("Unable to open MB I-BRAM device file");
        return false;
    }

    bram_i = (uint8_t *) mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_i, 0);

    if (bram_i == MAP_FAILED) {
        close(fd_i);
        LV_LOG_ERROR("Failed to mmap MB I-BRAM");
        return false;
    }

    /* D-BRAM */

    fd_d = open("/dev/uio2", O_RDWR | O_SYNC);

    if (fd_d < 1) {
        LV_LOG_ERROR("Unable to open MB D-BRAM device file");
        return false;
    }

    bram_d = (uint8_t *) mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_d, 0);

    if (bram_d == MAP_FAILED) {
        close(fd_d);
        LV_LOG_ERROR("Failed to mmap MB D-BRAM");
        return false;
    }

    mb_load(FW_DIR "mb.code", bram_i, true);
    mb_load(FW_DIR "mb.data", bram_d, false);

    /* * */

    control_mb_enable();
    control_fft_enable();

    pthread_t thread;

    pthread_create(&thread, NULL, mb_thread, NULL);
    pthread_detach(thread);

    return true;
}

void * mb_data() {
    return bram_d;
}
