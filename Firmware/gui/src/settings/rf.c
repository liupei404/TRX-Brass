/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include "rf.h"

static const char  filename[] = "/mnt/settings/rf.yaml";

rf_t   *rf = NULL;

static const cyaml_strval_t rf_mode_strings[] = {
    { "normal",     RF_NORMAL },
    { "two_tone",   RF_TWO_TONE },
    { "silence",    RF_SILENCE },
};

static const cyaml_schema_value_t uint_entry = {
    CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, uint16_t),
};

static const cyaml_schema_field_t calibrate_fields_schema[] = {
    CYAML_FIELD_INT("adc",          CYAML_FLAG_DEFAULT, rf_calibrate_t, adc),
    CYAML_FIELD_FLOAT("vpp",        CYAML_FLAG_DEFAULT, rf_calibrate_t, vpp),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t calibrate_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_FLOW, rf_calibrate_t, calibrate_fields_schema)
};

const cyaml_schema_field_t rf_swr_fields_schema[] = {
    CYAML_FIELD_INT("fwd_offset",           CYAML_FLAG_DEFAULT, rf_swr_t, fwd_offset),
    CYAML_FIELD_INT("rev_offset",           CYAML_FLAG_DEFAULT, rf_swr_t, rev_offset),
    CYAML_FIELD_SEQUENCE_COUNT("calibrate", CYAML_FLAG_POINTER, rf_swr_t, calibrate, count, &calibrate_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

const cyaml_schema_field_t rf_fields_schema[] = {
    CYAML_FIELD_ENUM("mode",                CYAML_FLAG_DEFAULT, rf_t, mode, rf_mode_strings, CYAML_ARRAY_LEN(rf_mode_strings)),
    CYAML_FIELD_SEQUENCE_FIXED("pa_bias",   CYAML_FLAG_DEFAULT,  rf_t, pa_bias, &uint_entry, 2),
    CYAML_FIELD_SEQUENCE_FIXED("two_tone",  CYAML_FLAG_DEFAULT,  rf_t, two_tone, &uint_entry, 2),
    CYAML_FIELD_SEQUENCE("rx_band",         CYAML_FLAG_POINTER,  rf_t, rx_band, &rx_band_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("tx_band",         CYAML_FLAG_POINTER,  rf_t, tx_band, &tx_band_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("xvrt",            CYAML_FLAG_POINTER,  rf_t, xvrt, &xvrt_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("txo_offset",           CYAML_FLAG_DEFAULT,  rf_t, txo_offset),
    CYAML_FIELD_FLOAT("ref_pwr",            CYAML_FLAG_DEFAULT,  rf_t, ref_pwr),
    CYAML_FIELD_FLOAT("pwr",                CYAML_FLAG_DEFAULT,  rf_t, pwr),
    CYAML_FIELD_UINT("ant",                 CYAML_FLAG_DEFAULT,  rf_t, ant),
    CYAML_FIELD_BOOL("atu",                 CYAML_FLAG_DEFAULT,  rf_t, atu),
    CYAML_FIELD_MAPPING("swr",              CYAML_FLAG_DEFAULT,  rf_t, swr, rf_swr_fields_schema),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t rf_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, rf_t, rf_fields_schema)
};

static const cyaml_config_t config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

void settings_rf_load() {
    cyaml_err_t err;

    err = cyaml_load_file(filename, &config, &rf_schema, (void **) &rf, NULL);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}

void settings_rf_save() {
    cyaml_err_t err;

    err = cyaml_save_file(filename, &config, &rf_schema, rf, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}
