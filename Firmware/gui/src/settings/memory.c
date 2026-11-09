/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include <cyaml/cyaml.h>
#include "lvgl/lvgl.h"
#include "memory.h"

static const cyaml_schema_field_t memory_item_fields_schema[] = {
    CYAML_FIELD_STRING_PTR("label",     CYAML_FLAG_POINTER, memory_item_t, label, 0, CYAML_UNLIMITED),
    CYAML_FIELD_MAPPING("work",         CYAML_FLAG_OPTIONAL | CYAML_FLAG_FLOW, memory_item_t, work, op_work_fields_schema),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t memory_item_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, memory_t, memory_item_fields_schema)
};

static const cyaml_schema_field_t memory_fields_schema[] = {
    CYAML_FIELD_SEQUENCE_COUNT("memory", CYAML_FLAG_POINTER, memory_t, item, count, &memory_item_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t memory_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, memory_t, memory_fields_schema)
};

static const cyaml_config_t config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

memory_t * settings_memory_load(const char *filename) {
    cyaml_err_t err;
    memory_t    *memory = NULL;

    err = cyaml_load_file(filename, &config, &memory_schema, (void **) &memory, NULL);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
    }

    return memory;
}

void settings_memory_save(const char *filename, memory_t *memory) {
    cyaml_err_t err;

    err = cyaml_save_file(filename, &config, &memory_schema, memory, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}

void settings_memory_free(memory_t *memory) {
    cyaml_err_t err;

    err = cyaml_free(&config, &memory_schema, memory, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
        return;
    }
}
