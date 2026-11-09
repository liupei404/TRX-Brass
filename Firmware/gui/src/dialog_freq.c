/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "lvgl/lvgl.h"
#include "dialog.h"
#include "dialog_freq.h"
#include "styles.h"
#include "dsp.h"
#include "events.h"
#include "util.h"
#include "keyboard.h"
#include "bands/bands.h"
#include "pannel.h"
#include "main_screen.h"
#include "msg.h"
#include "textarea_window.h"

static lv_obj_t *text;

static void construct_cb(lv_obj_t *parent);
static void destruct_cb();
static void key_cb(lv_event_t * e);

static dialog_t             dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = NULL,
    .buttons = true,
    .key_cb = key_cb
};

dialog_t                    *dialog_freq = &dialog;

static void edit_ok() {
    const char* str = textarea_window_get();

    if (strlen(str) == 0) {
        voice_say_text_fmt("Frequency window has been closed");
        return;
    }

    uint64_t    f = atof(str) * 1000000L;

    if ((f > 500000 && f < 62000000) || (f > 144000000 && f < 146000000) || (f > 430000000 && f < 440000000)) {
        main_screen_set_freq(f);
        voice_say_text_fmt("Frequency has been set %s", str);
    } else {
        msg_set_text_fmt("Incorrect freq");
        voice_say_text_fmt("Incorrect frequency");
    }

    dialog.obj = NULL;
    dialog_destruct();
}

static edit_cancel() {
    dialog.obj = NULL;
    dialog_destruct();
}

static void construct_cb(lv_obj_t *parent) {
    dialog.obj = textarea_window_open(edit_ok, edit_cancel);

    lv_obj_t *text = textarea_window_text();

    lv_textarea_set_accepted_chars(text, "0123456789.");
    lv_textarea_set_max_length(text, 9);
    lv_textarea_set_placeholder_text(text, "Freq in MHz");
    lv_obj_add_event_cb(text, key_cb, LV_EVENT_KEY, NULL);

    lv_group_add_obj(keyboard_group, text);
}

static void destruct_cb() {
    textarea_window_close();
    dialog.obj = NULL;
}

static void key_cb(lv_event_t * e) {
    uint32_t key = *((uint32_t *)lv_event_get_param(e));

    switch (key) {
        case '0' ... '9':
            voice_delay_say_text_fmt("%c", (char) key);
            break;

        case '.':
            voice_delay_say_text_fmt("point");
            break;

        case LV_KEY_BACKSPACE:
            voice_delay_say_text_fmt("backspace");
            break;

        case LV_KEY_ESC:
            voice_say_text_fmt("Frequency window has been closed");
            dialog_destruct();
            break;

        default:
            dialog_key_cb(e);
            break;
    }
}
