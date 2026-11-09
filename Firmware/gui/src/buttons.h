/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include "lvgl/lvgl.h"

typedef enum {
    PAGE_VOL_1 = 0,

    PAGE_VOICE_1,
    PAGE_VOICE_2,

    PAGE_CW_1,
    PAGE_CW_2,
    PAGE_CW_3,

    PAGE_RTTY_1,
    PAGE_RTTY_2,

    PAGE_OLIVIA_1,
    PAGE_OLIVIA_2,

    PAGE_DMR_1,

    PAGE_APP_1,
    PAGE_APP_2,
} button_page_t;

typedef void (*hold_cb_t)(void *);

typedef struct {
    char            *label;
    char            *voice;
    lv_event_cb_t   press;
    hold_cb_t       hold;
    uint16_t        data;
} button_item_t;

void buttons_init();
bool buttons_insert(int index, lv_obj_t *button);
void buttons_load(uint8_t n, button_item_t *item);
void buttons_load_page(button_page_t page);
void buttons_unload_page();
void buttons_press(uint8_t n, bool hold);
void buttons_visible(bool on);

void buttons_vol();
void buttons_mfk();
void buttons_app();
