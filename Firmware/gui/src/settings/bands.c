/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <cyaml/cyaml.h>
#include "lvgl/lvgl.h"
#include "bands.h"
#include "src/bands/bands.h"

static const char  filename[] = "/mnt/settings/bands.yaml";

static const cyaml_schema_field_t band_settings_fields_schema[] = {
    CYAML_FIELD_STRING_PTR("label",     CYAML_FLAG_POINTER, band_settings_t, label, 0, CYAML_UNLIMITED),
    CYAML_FIELD_UINT("start",           CYAML_FLAG_DEFAULT, band_settings_t, start),
    CYAML_FIELD_UINT("stop",            CYAML_FLAG_DEFAULT, band_settings_t, stop),
    CYAML_FIELD_BOOL("tx",              CYAML_FLAG_OPTIONAL, band_settings_t, tx),
    CYAML_FIELD_BOOL("jump",            CYAML_FLAG_OPTIONAL, band_settings_t, jump),
    CYAML_FIELD_END
};

static const cyaml_schema_field_t band_fields_schema[] = {
    CYAML_FIELD_MAPPING("settings",     CYAML_FLAG_FLOW, band_t, settings, band_settings_fields_schema),
    CYAML_FIELD_MAPPING("work",         CYAML_FLAG_OPTIONAL | CYAML_FLAG_FLOW, band_t, work, op_work_fields_schema),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t band_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, band_t, band_fields_schema)
};

static const cyaml_schema_field_t bands_fields_schema[] = {
    CYAML_FIELD_MAPPING("initial",      CYAML_FLAG_OPTIONAL | CYAML_FLAG_FLOW, bands_t, initial, op_work_fields_schema),
    CYAML_FIELD_SEQUENCE_COUNT("bands", CYAML_FLAG_POINTER, bands_t, item, count, &band_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t bands_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, bands_t, bands_fields_schema)
};

static const cyaml_config_t config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

bands_t *bands = NULL;

static int compare_band(const void *p1, const void *p2) {
    band_t *b1 = (band_t *) p1;
    band_t *b2 = (band_t *) p2;

    return (b1->settings.start < b2->settings.start) ? -1 : 1;
}

void settings_bands_load() {
    cyaml_err_t err;

    err = cyaml_load_file(filename, &config, &bands_schema, (void **) &bands, NULL);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }

    qsort(bands->item, bands->count, sizeof(band_t), compare_band);
    op_work = &bands->initial;
}

void settings_bands_save() {
    cyaml_err_t err;

    bands->initial = *op_work;

    err = cyaml_save_file(filename, &config, &bands_schema, bands, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}
