/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "dialog.h"
#include "dialog_settings.h"
#include "styles.h"
#include "events.h"
#include "keyboard.h"
#include "dsp.h"
#include "settings/rf.h"
#include "hw/pa_bias.h"
#include "fpga/control.h"

static void construct_cb(lv_obj_t *parent);
static void destruct_cb();

static dialog_t     dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = NULL,
    .buttons = false,
    .key_cb = dialog_key_cb
};

dialog_t            *dialog_rf_settings = &dialog;

static void destruct_cb() {
    settings_rf_save();
}

/* Mode */

static void mode_update_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

    rf->mode = lv_dropdown_get_selected(obj);
}

static void make_mode() {
    lv_obj_t    *obj;

    /* Label */

    dialog_label(&dialog, false, "TX Mode");

    /* * */

    obj = dialog_dropdown(&dialog, 6);

    lv_dropdown_set_options(obj, " Normal \n Two Tone \n Silence");
    lv_dropdown_set_selected(obj, rf->mode);
    lv_obj_add_event_cb(obj, mode_update_cb, LV_EVENT_VALUE_CHANGED, NULL);
}


/* PA Bias item */

static void pa_bias_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    uint16_t        *x = lv_event_get_user_data(e);

    *x = lv_spinbox_get_value(obj);
    pa_bias_update();
}

static void make_pa_bias() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "PA Bias");

    for (int i = 0; i < 2; i++) {
        obj = lv_spinbox_create(dialog.grid);

        dialog_item(&dialog, obj, 3);

        lv_spinbox_set_value(obj, rf->pa_bias[i]);
        lv_spinbox_set_range(obj, 0, 255);
        lv_spinbox_set_digit_format(obj, 3, 0);
        lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

        lv_obj_add_event_cb(obj, pa_bias_update_cb, LV_EVENT_VALUE_CHANGED, &rf->pa_bias[i]);
    }
}

/* TX Band */

static void tx_band_freq_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    tx_band_item_t  *item = lv_event_get_user_data(e);

    item->to = lv_spinbox_get_value(obj) * 1000;
}

static void tx_band_vref_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    tx_band_item_t  *item = lv_event_get_user_data(e);

    item->vref = lv_spinbox_get_value(obj);
}

static void make_tx_band() {
    lv_obj_t    *obj;

    dialog_title(&dialog, "TX Band");

    for (int i = 0; i < rf->tx_band_count; i++) {
        uint8_t         col = 0;
        tx_band_item_t  *item = &rf->tx_band[i];

        /* Label */

        dialog_label(&dialog, true, "LPF:%i", item->lpf + 1);

        /* Freq */

        obj = lv_spinbox_create(dialog.grid);

        dialog_item(&dialog, obj, 3);

        lv_spinbox_set_value(obj, item->to / 1000);
        lv_spinbox_set_range(obj, 0, 62000);
        lv_spinbox_set_digit_format(obj, 6, 3);
        lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

        lv_obj_add_event_cb(obj, tx_band_freq_update_cb, LV_EVENT_VALUE_CHANGED, item);

        /* VRef */

        obj = lv_spinbox_create(dialog.grid);

        dialog_item(&dialog, obj, 3);

        lv_spinbox_set_value(obj, item->vref);
        lv_spinbox_set_range(obj, 0, 4095);
        lv_spinbox_set_digit_format(obj, 4, 0);
        lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

        lv_obj_add_event_cb(obj, tx_band_vref_update_cb, LV_EVENT_VALUE_CHANGED, item);
    }
}

/* TXO freq */

static void txo_freq_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);

    rf->txo_offset = lv_spinbox_get_value(obj);
    control_update();
}

static void make_txo_freq() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "TXO offset");

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 6);

    lv_spinbox_set_value(obj, rf->txo_offset);
    lv_spinbox_set_range(obj, -10000, 10000);
    lv_obj_add_event_cb(obj, txo_freq_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_spinbox_set_digit_format(obj, 6, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);
}

/* * */

static void construct_cb(lv_obj_t *parent) {
    dialog_grid(parent, &dialog);

    make_mode();
    make_pa_bias();
    make_txo_freq();
    make_tx_band();
}
