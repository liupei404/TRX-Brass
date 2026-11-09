/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#include "rotary.h"
#include "keyboard.h"
#include "backlight.h"
#include "main.h"

static void rotary_input_read(lv_indev_t *indev, lv_indev_data_t *data) {
    struct input_event  in;
    rotary_t            *rotary = lv_indev_get_user_data(indev);
    bool                send = false;

    while (read(rotary->fd, &in, sizeof(struct input_event)) > 0) {
        if (in.type == EV_REL) {
            rotary->accum += in.value;
            send = true;
        }
    }
    
    if (send) {
        int16_t diff = rotary->accum / rotary->div;
        rotary->accum = rotary->accum % rotary->div;

        if (diff != 0) {
            backlight_tick();
    
            if (rotary->left[0] == 0 && rotary->right[0] == 0) {
                lv_obj_send_event(lv_scr_act(), EVENT_ROTARY, (void *) (intptr_t) diff);
            } else {
                data->state = LV_INDEV_STATE_PRESSED;
                data->key = diff < 0 ? rotary->left[rotary->mode] : rotary->right[rotary->mode];
            }
        }
    }
}

rotary_t * rotary_init(char *dev_name, uint8_t div) {
    int fd = open(dev_name, O_RDWR | O_NOCTTY | O_NDELAY);

    if (fd == -1) {
        LV_LOG_ERROR("unable to open rotary interface:");

        return NULL;
    }

    fcntl(fd, F_SETFL, O_ASYNC | O_NONBLOCK);

    rotary_t *rotary = malloc(sizeof(rotary_t));
    
    memset(rotary, 0, sizeof(rotary_t));
    rotary->fd = fd;
    rotary->div = div;
    rotary->accum = 0;
    
    rotary->indev = lv_indev_create();
    lv_indev_set_type(rotary->indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(rotary->indev, rotary_input_read);
    lv_indev_set_user_data(rotary->indev, rotary);

    lv_indev_set_group(rotary->indev, keyboard_group);
    
    return rotary;
}
