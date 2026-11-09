/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <stdlib.h>
#include <cyaml/cyaml.h>
#include "lvgl/lvgl.h"
#include "sstv.h"

static const char filename[] = "/mnt/settings/app/sstv.yaml";

static const cyaml_schema_field_t fields[] = {
    CYAML_FIELD_UINT("partial_min_percent", CYAML_FLAG_OPTIONAL, settings_sstv_t, partial_min_percent),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, settings_sstv_t, fields)
};

static const cyaml_config_t config = {
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
    .log_level = CYAML_LOG_WARNING,
};

settings_sstv_t *settings_sstv;

void settings_sstv_load(void) {
    if (cyaml_load_file(filename, &config, &schema, (void **) &settings_sstv, NULL) != CYAML_OK) {
        settings_sstv = calloc(1, sizeof(*settings_sstv));
        if (settings_sstv) settings_sstv->partial_min_percent = 25;
    }

    if (settings_sstv && settings_sstv->partial_min_percent > 100) {
        settings_sstv->partial_min_percent = 25;
    }
}

void settings_sstv_save(void) {
    if (!settings_sstv) {
        return;
    }

    cyaml_err_t err = cyaml_save_file(filename, &config, &schema, settings_sstv, 0);

    if (err != CYAML_OK) {
        LV_LOG_ERROR("%s", cyaml_strerror(err));
    }

    cyaml_free(&config, &schema, settings_sstv, 0);
    settings_sstv = NULL;
}
