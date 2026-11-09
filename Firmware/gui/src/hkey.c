/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "settings/options.h"
#include "settings/hw.h"
#include "events.h"
#include "hkey.h"
#include "util.h"
#include "queue.h"
#include "main_screen.h"
#include "keyboard.h"

static uint8_t      hist = 0;
static int16_t      hist_x[3] = {-1, -1, -1};
static int16_t      hist_y[3] = {-1, -1, -1};
static hkey_state_t state = HKEY_RELEASE;
static uint64_t     press_time;
static hkey_t       key = HKEY_NONE;
static hmic_item_t  *hmic = NULL;

static const hkey_t matrix[7][5] = {
    { HKEY_NONE, HKEY_1,    HKEY_2,    HKEY_3,      HKEY_A    },
    { HKEY_NONE, HKEY_4,    HKEY_5,    HKEY_6,      HKEY_B    },
    { HKEY_UP,   HKEY_NONE, HKEY_NONE, HKEY_NONE,   HKEY_NONE },
    { HKEY_NONE, HKEY_7,    HKEY_8,    HKEY_9,      HKEY_C    },
    { HKEY_DOWN, HKEY_NONE, HKEY_NONE, HKEY_NONE,   HKEY_NONE },
    { HKEY_NONE, HKEY_STAR, HKEY_0,    HKEY_SHARP,  HKEY_D    },
    { HKEY_NONE, HKEY_P1,   HKEY_P2,   HKEY_P3,     HKEY_P4   }
};

static void hkey_send_key(int32_t key) {
    int32_t *c = malloc(sizeof(int32_t));
    *c = key;

    queue_send(lv_group_get_focused(keyboard_group), LV_EVENT_KEY, c);
}

static void hkey_send(hkey_state_t event) {
    if (!lv_group_get_editing(keyboard_group)) {
        if (event == HKEY_PRESS) {
            switch (key) {
                case HKEY_0 ... HKEY_9:
                    hkey_send_key(key - HKEY_0 + '0');
                    break;

                case HKEY_STAR:
                    hkey_send_key('.');
                    break;

                case HKEY_SHARP:
                    hkey_send_key(LV_KEY_BACKSPACE);
                    break;

                default:
                    break;
            }
        }

        return;
    }

    event_hkey_t    *e = malloc(sizeof(event_hkey_t));

    e->key = key;
    e->state = event;

    if (main_screen_ready()) {
        queue_send(lv_scr_act(), EVENT_HKEY, (void*) e);
    }
}

void hkey_put(uint16_t x, uint16_t y) {
    int16_t xi = -1;
    int16_t yi = -1;

    if (hmic == NULL) {
        return;
    }

    for (int i = 0; i < 7; i++) {
        if (x < hmic->x[i]) {
            xi = i;
            break;
        }
    }

    for (int i = 0; i < 5; i++) {
        if (y < hmic->y[i]) {
            yi = i;
            break;
        }
    }

    hist_x[hist] = xi;
    hist_y[hist] = yi;

    hist = (hist + 1) % 3;

    for (int i = 0; i < 2; i++)
        if (hist_x[i] != hist_x[i + 1])
            return;

    for (int i = 0; i < 2; i++)
        if (hist_y[i] != hist_y[i + 1])
            return;

    uint64_t now = get_time();

    if (xi >= 0 && yi >= 0) {
        switch (state) {
            case HKEY_RELEASE:
                state = HKEY_PRESS;
                key = matrix[xi][yi];
                press_time = now;
                hkey_send(HKEY_PRESS);
                break;

            case HKEY_PRESS:
                if (now - press_time > 1000) {
                    state = HKEY_LONG;
                    hkey_send(HKEY_LONG);
                }
                break;

            default:
                break;
        }
    } else {
        switch (state) {
            case HKEY_PRESS:
                state = HKEY_RELEASE;
                hkey_send(HKEY_RELEASE);
                key = HKEY_NONE;
                break;

            case HKEY_LONG:
                state = HKEY_RELEASE;
                hkey_send(HKEY_LONG_RELEASE);
                key = HKEY_NONE;
                break;

            default:
                break;
        }
    }
}

void hkey_mic_update() {
     for (uint16_t i = 0; i < hw->hmic_count; i++) {
        hmic_item_t *item = &hw->hmic[i];

        if (strcmp(item->label, options->hkeys.hmic) == 0) {
            hmic = item;
            return;
        }
     }

     hmic = NULL;
}
