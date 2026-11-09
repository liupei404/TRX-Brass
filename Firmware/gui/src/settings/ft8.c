/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <cyaml/cyaml.h>
#include "lvgl/lvgl.h"
#include "ft8.h"

static const char  filename[] = "/mnt/settings/app/ft8.yaml";

static const cyaml_strval_t protocol_strings[] = {
    { "ft8",    PROTO_FT8 },
    { "ft4",    PROTO_FT4 },
};

static const cyaml_strval_t show_strings[] = {
    { "all",    FT8_SHOW_ALL },
    { "cq",     FT8_SHOW_CQ },
};

const cyaml_schema_field_t ft8_fields_schema[] = {
    CYAML_FIELD_ENUM("protocol",        CYAML_FLAG_OPTIONAL, settings_ft8_t, protocol, protocol_strings, CYAML_ARRAY_LEN(protocol_strings)),
    CYAML_FIELD_UINT("ft8_band",        CYAML_FLAG_OPTIONAL, settings_ft8_t, ft8_band),
    CYAML_FIELD_UINT("ft4_band",        CYAML_FLAG_OPTIONAL, settings_ft8_t, ft4_band),
    CYAML_FIELD_ENUM("show",            CYAML_FLAG_OPTIONAL, settings_ft8_t, show, show_strings, CYAML_ARRAY_LEN(show_strings)),
    CYAML_FIELD_BOOL("tx_auto",         CYAML_FLAG_OPTIONAL, settings_ft8_t, tx_auto),
    CYAML_FIELD_UINT("tx_freq",         CYAML_FLAG_OPTIONAL, settings_ft8_t, tx_freq),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t ft8_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, settings_ft8_t, ft8_fields_schema)
};

static const cyaml_config_t config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

settings_ft8_t   *settings_ft8 = NULL;

void settings_ft8_load() {
    cyaml_err_t err;

    err = cyaml_load_file(filename, &config, &ft8_schema, (void **) &settings_ft8, NULL);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}

void settings_ft8_save() {
    cyaml_err_t err;

    err = cyaml_save_file(filename, &config, &ft8_schema, settings_ft8, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }

    err = cyaml_free(&config, &ft8_schema, settings_ft8, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }

    settings_ft8 = NULL;
}
