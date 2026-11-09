/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stddef.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "bands.h"
#include "tree.h"
#include "src/settings/bands.h"
#include "src/settings/op_work.h"
#include "src/settings/modes.h"
#include "src/msgs.h"
#include "src/voice.h"

static band_tree_t  *tree;
static band_list_t  *list = NULL;

static op_work_t  op_dummy = {
    .rx = 14100000,
    .tx = 14100000,
    .fft = 14100000,
    .mode = RADIO_MODE_USB,
    .split = SPLIT_NONE
};

band_settings_t     *band_settings = NULL;

static void reload_cb(void *s, brass_msg_t *m) {
    uint8_t     zoom = op_mode->spectrum_factor;
    uint64_t    freq = op_work->fft;
    uint64_t    start = freq - 100000 / 2 / zoom;
    uint64_t    stop = freq + 100000 / 2 / zoom;

    if (list) {
        band_list_delete(list);
        list = NULL;
    }

    list = band_tree_findall(tree, start, stop);
}

static void rx_pre_changed_cb(void *s, brass_msg_t *m) {
    const uint64_t  *freq = brass_msg_get_payload(m);
    band_t          *band = band_tree_find(tree, *freq, *freq);

    if (band) {
        if (&band->settings != band_settings) {
            bands_activate(band, false);
        }
    } else {
        if (op_work != &op_dummy) {
            band_settings = NULL;
            op_dummy = *op_work;
            op_work = &op_dummy;
        }
    }
}

void bands_init() {
    tree = band_tree_new();

    for (size_t i = 0; i < bands->count; i++)
        band_tree_insert(tree, &bands->item[i]);

    brass_msg_subscribe(MSG_RATE_FFT_CHANGED, reload_cb, NULL);
    brass_msg_subscribe(MSG_FREQ_FFT_CHANGED, reload_cb, NULL);
    brass_msg_subscribe(MSG_FREQ_RX_PRE_CHANGED, rx_pre_changed_cb, NULL);

    band_t *band = band_tree_find(tree, op_work->rx, op_work->rx);

    if (band) {
        band_settings = &band->settings;
        op_work = &band->work;
    }
}

band_list_t * bands_list() {
    return list;
}

void bands_activate(band_t *band, bool touch_freq) {
    band_settings = &band->settings;
    op_work = &band->work;

    brass_msg_send(MSG_BAND_CHANGED, NULL);

    if (touch_freq) {
        if (op_work->rx == 0) {
            op_work->rx = band_settings->start / 2 + band_settings->stop / 2;
        }

        if (op_work->tx == 0) {
            op_work->tx = band_settings->start / 2 + band_settings->stop / 2;
        }

        if (op_work->fft == 0) {
            op_work->fft = band_settings->start / 2 + band_settings->stop / 2;
        }
    }

    op_work_activate(touch_freq);
    brass_msg_send(MSG_MODE_CHANGED, &op_work->mode);
}

void bands_change(bool up) {
    uint64_t    freq = op_work->rx;

    if (up) {
        for (int32_t i = 0; i < bands->count; i++) {
            band_t *band = &bands->item[i];

            if (band->settings.jump && band->settings.start > freq) {
                bands_activate(band, true);

                if (band_settings->label) {
                    voice_say_text_fmt("Band %s", band_settings->label);
                }
                break;
            }
        }
    } else {
        for (int32_t i = bands->count - 1; i >= 0; i--) {
            band_t *band = &bands->item[i];

            if (band->settings.jump && band->settings.stop < freq) {
                bands_activate(band, true);

                if (band_settings->label) {
                    voice_say_text_fmt("Band %s", band_settings->label);
                }
                break;
            }
        }
    }
}
