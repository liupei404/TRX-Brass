/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include "src/radio.h"
#include "src/msgs.h"

#include "op_work.h"
#include "modes.h"

static const cyaml_strval_t radio_mode_strings[] = {
    { "lsb",    RADIO_MODE_LSB },
    { "usb",    RADIO_MODE_USB },
    { "cw",     RADIO_MODE_CW },
    { "cwr",    RADIO_MODE_CWR },
    { "am",     RADIO_MODE_AM },
    { "nfm",    RADIO_MODE_NFM },
    { "rtty",   RADIO_MODE_RTTY },
    { "olivia", RADIO_MODE_OLIVIA }
};

static const cyaml_strval_t split_mode_strings[] = {
    { "None",   SPLIT_NONE },
    { "Rx",     SPLIT_RX },
    { "Tx",     SPLIT_TX }
};

const cyaml_schema_field_t op_work_fields_schema[] = {
    CYAML_FIELD_UINT("rx",              CYAML_FLAG_OPTIONAL, op_work_t, rx),
    CYAML_FIELD_UINT("tx",              CYAML_FLAG_OPTIONAL, op_work_t, tx),
    CYAML_FIELD_UINT("fft",             CYAML_FLAG_OPTIONAL, op_work_t, fft),
    CYAML_FIELD_ENUM("mode",            CYAML_FLAG_OPTIONAL, op_work_t, mode, radio_mode_strings, CYAML_ARRAY_LEN(radio_mode_strings)),
    CYAML_FIELD_ENUM("split",           CYAML_FLAG_OPTIONAL, op_work_t, split, split_mode_strings, CYAML_ARRAY_LEN(split_mode_strings)),
    CYAML_FIELD_END
};

op_work_t *op_work;
op_work_t *op_work_backup;

void op_work_activate(bool set_freq) {
    if (set_freq) {
        radio_set_freq_rx(op_work->rx);
        radio_set_freq_tx(op_work->tx);
        radio_set_freq_fft(op_work->fft);
    }

    settings_mode_update(op_work->mode);
}

void op_work_set_mode(radio_mode_t mode) {
    if (mode != op_work->mode) {
        op_work->mode = mode;
        settings_mode_update(op_work->mode);
        brass_msg_send(MSG_MODE_CHANGED, &op_work->mode);
    }
}

void op_work_keep() {
    op_work_backup = op_work;
}

void op_work_restore() {
    op_work = op_work_backup;
    op_work_activate(true);
}
