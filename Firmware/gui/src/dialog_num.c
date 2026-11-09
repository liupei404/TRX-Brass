/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include "textarea_window.h"
#include "main_screen.h"
#include "msg.h"
#include "main.h"
#include "dialog.h"
#include "events.h"

static uint32_t     min_val;
static uint32_t     max_val;
static uint32_t     *ptr_val;
static const char   *title_val;

static void construct_cb(lv_obj_t *parent);
static void destruct_cb();

static dialog_t             dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = NULL,
    .buttons = true,
    .key_cb = dialog_key_cb
};

static void edit_ok() {
    const char *str = textarea_window_get();
    uint32_t    n = atoi(str);

    if (n >= min_val && n <= max_val) {
        *ptr_val = n;
    } else {
        msg_set_text_fmt("Invalid value");
    }

    dialog.obj = NULL;
    dialog_destruct();
}

static void edit_cancel() {
    dialog.obj = NULL;
    dialog_destruct();
}

static void construct_cb(lv_obj_t *parent) {
    dialog.obj = textarea_window_open(edit_ok, edit_cancel);

    lv_obj_t *text = textarea_window_text();

    lv_textarea_set_accepted_chars(text, "0123456789");

    lv_textarea_set_max_length(text, 8);
    lv_textarea_set_placeholder_text(text, title_val);
    lv_obj_add_event_cb(text, dialog_key_cb, LV_EVENT_KEY, NULL);

    char str[16];

    snprintf(str, sizeof(str) - 1, "%i", *ptr_val);
    textarea_window_set(str);
}

static void destruct_cb() {
    textarea_window_close();
    dialog.obj = NULL;
}

void dialog_num_open(uint32_t min, uint32_t max, uint32_t *val, const char *title) {
    min_val = min;
    max_val = max;
    ptr_val = val;
    title_val = title;

    dialog_construct(&dialog, NULL);
}
