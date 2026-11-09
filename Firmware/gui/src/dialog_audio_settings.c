/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "lvgl/lvgl.h"
#include "dialog.h"
#include "dialog_settings.h"
#include "styles.h"
#include "events.h"
#include "keyboard.h"
#include "dsp.h"
#include "mic.h"
#include "audio.h"
#include "settings/options.h"

static void construct_cb(lv_obj_t *parent);
static void destruct_cb(void);
static void equalizer_speaker_update_cb(lv_event_t * e);
static void equalizer_mic_update_cb(lv_event_t * e);

typedef enum {
    EQ_SPEAKER = 0,
    EQ_MIC
} eq_sel_t;

static const lv_event_cb_t  eq_callbacks[] = { equalizer_speaker_update_cb, equalizer_mic_update_cb };

static dialog_t     dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = NULL,
    .buttons = false,
    .key_cb = dialog_key_cb
};

dialog_t            *dialog_audio_settings = &dialog;

typedef struct {
    bool                capture;
    audio_device_list_t devices;
    uint16_t            selected;
} device_selector_t;

static device_selector_t mic_selector = { .capture = true };
static device_selector_t speaker_selector;

static void destruct_cb(void) {
    audio_free_devices(&mic_selector.devices);
    audio_free_devices(&speaker_selector.devices);
    settings_options_save();
}

static void device_update_cb(lv_event_t *e) {
    lv_obj_t            *obj = lv_event_get_target(e);
    device_selector_t   *selector = lv_event_get_user_data(e);
    uint16_t            selected = lv_dropdown_get_selected(obj);

    if (selected == selector->selected)
        return;

    /* The final entry can be a saved device which is currently unavailable. */

    if (selected > selector->devices.count) {
        lv_dropdown_set_selected(obj, selector->selected);
        return;
    }

    const char  *name = selected ? selector->devices.items[selected - 1].name : "";
    char        *device = strdup(name);

    if (!device || !audio_set_device(selector->capture, name)) {
        free(device);
        lv_dropdown_set_selected(obj, selector->selected);
        return;
    }

    char **setting = selector->capture ? &options->audio.mic.device : &options->audio.speaker.device;

    free(*setting);

    *setting = device;
    selector->selected = selected;
}

static bool add_device_option(lv_obj_t *obj, const char *description, const char *name) {
    size_t  size = strlen(description) + strlen(name) + 4;
    char    *label = malloc(size);

    if (!label)
        return false;

    snprintf(label, size, "%s (%s)", description, name);

    /* Each device must occupy exactly one dropdown row. */

    for (char *p = label; *p; p++) {
        if (*p == '\n' || *p == '\r')
            *p = ' ';
    }

    lv_dropdown_add_option(obj, label, LV_DROPDOWN_POS_LAST);
    free(label);
    return true;
}

static void make_device_selector(device_selector_t *selector) {
    dialog_label(&dialog, true, "Device");

    lv_obj_t *obj = dialog_dropdown(&dialog, 6);

    lv_dropdown_set_options(obj, "Default");
    selector->selected = 0;

    bool        available = audio_get_devices(selector->capture, &selector->devices);
    const char  *saved = selector->capture ? options->audio.mic.device : options->audio.speaker.device;

    for (size_t i = 0; i < selector->devices.count; i++) {
        audio_device_t *device = &selector->devices.items[i];

        if (!add_device_option(obj, device->description, device->name)) {
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            return;
        }

        if (saved && !strcmp(saved, device->name))
            selector->selected = i + 1;
    }

    if (saved && *saved && !selector->selected) {
        if (!add_device_option(obj, "Unavailable", saved)) {
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            return;
        }
        selector->selected = selector->devices.count + 1;
    }

    lv_dropdown_set_selected(obj, selector->selected);

    if (!available) {
        LV_LOG_WARN("Cannot enumerate PulseAudio devices");
        lv_obj_add_state(obj, LV_STATE_DISABLED);
    }

    lv_obj_add_event_cb(obj, device_update_cb, LV_EVENT_VALUE_CHANGED, selector);
}

/* Equalizer item */

static void equalizer_speaker_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    int16_t         *x = lv_event_get_user_data(e);

    *x = lv_spinbox_get_value(obj);
    dsp_update_equalizer();
}

static void equalizer_mic_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    int16_t         *x = lv_event_get_user_data(e);
    int16_t         previous = *x;

    *x = lv_spinbox_get_value(obj);

    eq_config_t config = settings_mic_eq_config(&options->audio.mic);

    if (!eq_valid_config(&config)) {
        *x = previous;
        lv_spinbox_set_value(obj, previous);
        return;
    }

    mic_update_equalizer();
}

static void make_equalizer_item(eq_sel_t sel, uint8_t n, equalizer_item_t *item) {
    lv_obj_t        *obj;

    /* Label */

    dialog_label(&dialog, true, "EQ band %i", n + 1);

    /* Freq */

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, item->freq);

    switch (n) {
        case 0:
            lv_spinbox_set_range(obj, 50, 800);
            lv_spinbox_set_digit_format(obj, 3, 0);
            break;
        case 1:
            lv_spinbox_set_range(obj, 800, 1600);
            lv_spinbox_set_digit_format(obj, 4, 0);
            break;
        case 2:
            lv_spinbox_set_range(obj, 1600, 3200);
            lv_spinbox_set_digit_format(obj, 4, 0);
            break;
    }

    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    /* Width */

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, item->q);
    lv_spinbox_set_range(obj, 1, 9);
    lv_spinbox_set_digit_format(obj, 1, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, eq_callbacks[sel], LV_EVENT_VALUE_CHANGED, &item->q);

    /* Gain */

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, item->gain);
    lv_spinbox_set_range(obj, -24, 12);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);
    lv_obj_add_event_cb(obj, eq_callbacks[sel], LV_EVENT_VALUE_CHANGED, &item->gain);
}

static void make_mic_point(uint8_t n) {
    eq_point_t *point = &options->audio.mic.eq[n];

    dialog_label(&dialog, true, "EQ point %i (Hz/dB)", n + 1);

    lv_obj_t *obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 3);

    lv_spinbox_set_range(obj, 0, 6399);
    lv_spinbox_set_digit_format(obj, 4, 0);
    lv_spinbox_set_value(obj, point->freq);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);
    lv_obj_add_event_cb(obj, equalizer_mic_update_cb, LV_EVENT_VALUE_CHANGED, &point->freq);

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 3);

    lv_spinbox_set_range(obj, -24, 12);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_value(obj, point->gain);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);
    lv_obj_add_event_cb(obj, equalizer_mic_update_cb, LV_EVENT_VALUE_CHANGED, &point->gain);
}

/* Mic filter */

static void mic_filter_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    uint16_t        *x = lv_event_get_user_data(e);
    uint16_t        previous = *x;

    *x = lv_spinbox_get_value(obj);

    eq_config_t config = settings_mic_eq_config(&options->audio.mic);

    if (!eq_valid_config(&config)) {
        *x = previous;
        lv_spinbox_set_value(obj, previous);
        return;
    }

    mic_update_filter();
}

static void make_mic_filter() {
    lv_obj_t        *obj;
    filter_t        *filter = &options->audio.mic.filter;

    /* Label */

    dialog_label(&dialog, true, "Filter");

    /* Low */

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, filter->low);
    lv_spinbox_set_range(obj, 50, 800);
    lv_spinbox_set_digit_format(obj, 3, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, mic_filter_update_cb, LV_EVENT_VALUE_CHANGED, &filter->low);

    /* High */

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, filter->high);
    lv_spinbox_set_range(obj, 2000, 3200);
    lv_spinbox_set_digit_format(obj, 4, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, mic_filter_update_cb, LV_EVENT_VALUE_CHANGED, &filter->high);

    /* Transition */

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, filter->transition);
    lv_spinbox_set_range(obj, 50, 200);
    lv_spinbox_set_digit_format(obj, 3, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, mic_filter_update_cb, LV_EVENT_VALUE_CHANGED, &filter->transition);
}

/* NR */

static void nr_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    uint16_t        *x = lv_event_get_user_data(e);

    *x = lv_spinbox_get_value(obj);
    dsp_update_denoise();
}

static void nr_item(const char *label, int16_t *data, int16_t min, int16_t max) {
    lv_obj_t    *obj;

    dialog_label(&dialog, true, label);

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 6);
    lv_spinbox_set_value(obj, *data);
    lv_spinbox_set_range(obj, min, max);
    lv_obj_add_event_cb(obj, nr_update_cb, LV_EVENT_VALUE_CHANGED, data);

    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);
}

static void nr_scaling_type_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

    options->audio.denoise.nr.noise_scaling_type = lv_dropdown_get_selected(obj);

    dsp_update_denoise();
}

static void make_nr() {
    lv_obj_t    *obj;

    dialog_title(&dialog, "Noice reducer");

    nr_item("Reduction amount", &options->audio.denoise.nr.reduction_amount, 0, 20);
    nr_item("Smoothing factor", &options->audio.denoise.nr.smoothing_factor, 0, 100);
    nr_item("Whitening factor", &options->audio.denoise.nr.whitening_factor, 0, 100);

    /* * */

    dialog_label(&dialog, true, "Noise scale type");

    obj = dialog_dropdown(&dialog, 6);

    lv_dropdown_set_options(obj, " Complite spectrum \n Critical bands \n Masking thresholds ");
    lv_dropdown_set_selected(obj, options->audio.denoise.nr.noise_scaling_type);
    lv_obj_add_event_cb(obj, nr_scaling_type_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    nr_item("Noise rescale", &options->audio.denoise.nr.noise_rescale, 0, 12);
    nr_item("Threshold", &options->audio.denoise.nr.post_filter_threshold, -10, 10);
}

/* EMNR */

static void emnr_method_cb(lv_event_t * e) {
    lv_obj_t    *obj = lv_event_get_target(e);
    uint16_t    *x = lv_event_get_user_data(e);

    *x = lv_dropdown_get_selected(obj);

    dsp_update_denoise();
}

static void emnr_trained_cb(lv_event_t * e) {
    lv_obj_t    *obj = lv_event_get_target(e);
    float       *x = lv_event_get_user_data(e);

    *x = lv_spinbox_get_value(obj) / 100.0f;

    dsp_update_denoise();
}

static void make_emnr() {
    lv_obj_t    *obj;
    lv_obj_t    *list;

    dialog_title(&dialog, "EMNR");

    dialog_label(&dialog, true, "Gain method");

    obj = dialog_dropdown(&dialog, 6);

    lv_dropdown_set_options(obj, " Gausian, linear scale \n Gausian, log scale \n Gamma \n Trained ");
    lv_dropdown_set_selected(obj, options->audio.denoise.emnr.gain_method);
    lv_obj_add_event_cb(obj, emnr_method_cb, LV_EVENT_VALUE_CHANGED, &options->audio.denoise.emnr.gain_method);

    /* * */

    dialog_label(&dialog, true, "NPE method");

    obj = dialog_dropdown(&dialog, 6);

    lv_dropdown_set_options(obj, " OSMS \n MMSE \n NSTAT ");
    lv_dropdown_set_selected(obj, options->audio.denoise.emnr.npe_method);
    lv_obj_add_event_cb(obj, emnr_method_cb, LV_EVENT_VALUE_CHANGED, &options->audio.denoise.emnr.npe_method);

    /* * */

    dialog_label(&dialog, true, "Trained threshold");

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 6);
    lv_spinbox_set_value(obj, options->audio.denoise.emnr.trained_thresh * 100);
    lv_spinbox_set_range(obj, -500, 500);
    lv_obj_add_event_cb(obj, emnr_trained_cb, LV_EVENT_VALUE_CHANGED, &options->audio.denoise.emnr.trained_thresh);

    lv_spinbox_set_digit_format(obj, 3, 1);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    /* * */

    dialog_label(&dialog, true, "Trained T2");

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 6);
    lv_spinbox_set_value(obj, options->audio.denoise.emnr.trained_t2 * 100);
    lv_spinbox_set_range(obj, 2, 30);
    lv_obj_add_event_cb(obj, emnr_trained_cb, LV_EVENT_VALUE_CHANGED, &options->audio.denoise.emnr.trained_t2);

    lv_spinbox_set_digit_format(obj, 3, 1);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);
}

/* * */

static void construct_cb(lv_obj_t *parent) {
    dialog_grid(parent, &dialog);

    /* * */

    dialog_title(&dialog, "Mic");
    make_device_selector(&mic_selector);

    make_mic_filter();

    for (uint8_t i = 0; i < EQ_MAX_POINTS; i++) {
        make_mic_point(i);
    }

    /* * */

    dialog_title(&dialog, "Speaker");
    make_device_selector(&speaker_selector);

    for (uint8_t i = 0; i < EQUALIZER_NUM; i++) {
        make_equalizer_item(EQ_SPEAKER, i, &options->audio.speaker.eq[i]);
    }

    /* * */

    make_nr();
    make_emnr();
}
