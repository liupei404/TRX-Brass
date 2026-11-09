/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "textarea_window.h"
#include "main_screen.h"
#include "dialog.h"
#include "events.h"
#include "dsp.h"
#include "settings/options.h"

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

dialog_t                    *dialog_callsign = &dialog;

static void edit_ok() {
    strcpy(options->op.callsign, textarea_window_get());
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

    lv_textarea_set_accepted_chars(text, 
        "0123456789/"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    );

    lv_textarea_set_max_length(text, sizeof(options->op.callsign) - 1);
    lv_textarea_set_placeholder_text(text, "Callsign");
    lv_obj_add_event_cb(text, dialog_key_cb, LV_EVENT_KEY, NULL);

    textarea_window_set(options->op.callsign);
}

static void destruct_cb() {
    textarea_window_close();
    dialog.obj = NULL;
}
