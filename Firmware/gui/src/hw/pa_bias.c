/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "pa_bias.h"
#include "src/settings/rf.h"

static void write_to(const char *path, int32_t data) {
    int fd = open(path, O_WRONLY);

    if (fd > 0) {
        char    buf[32];
        int     size = snprintf(buf, sizeof(buf), "%i\n", data);

        if (write(fd, buf, size) != size) {
            LV_LOG_ERROR("Problem write %s", path);
        }

        close(fd);
    } else {
        LV_LOG_ERROR("Problem open %s", path);
    }
}

void pa_bias_update() {
    write_to("/sys/bus/i2c/devices/0-002c/rdac0", rf->pa_bias[0]);
    write_to("/sys/bus/i2c/devices/0-002d/rdac0", rf->pa_bias[1]);
}
