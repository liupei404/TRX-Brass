/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include "hw.h"
#include "src/hw/xvrt.h"

static const char  filename[] = "/mnt/settings/hw.yaml";

hw_t   *hw = NULL;

static const cyaml_schema_value_t uint_entry = {
    CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, uint16_t),
};

static const cyaml_schema_field_t hmic_item_schema[] = {
    CYAML_FIELD_STRING_PTR("label",     CYAML_FLAG_POINTER, hmic_item_t, label, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE_FIXED("x",     CYAML_FLAG_DEFAULT, hmic_item_t, x, &uint_entry, 7),
    CYAML_FIELD_SEQUENCE_FIXED("y",     CYAML_FLAG_DEFAULT, hmic_item_t, y, &uint_entry, 5),

    CYAML_FIELD_END
};

const cyaml_schema_value_t hmic_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_FLOW, hmic_item_t, hmic_item_schema)
};

const cyaml_schema_field_t hw_fields_schema[] = {
    CYAML_FIELD_SEQUENCE("hmic",        CYAML_FLAG_POINTER,  hw_t, hmic, &hmic_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_UINT("tcxo_freq",       CYAML_FLAG_DEFAULT,  hw_t, tcxo_freq),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t hw_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, hw_t, hw_fields_schema)
};

static const cyaml_config_t config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

void settings_hw_load() {
    cyaml_err_t err;

    err = cyaml_load_file(filename, &config, &hw_schema, (void **) &hw, NULL);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }

    xvrt_load_tcxo_freq();
}

void settings_hw_save() {
    cyaml_err_t err;

    err = cyaml_save_file(filename, &config, &hw_schema, hw, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}
