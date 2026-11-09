/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "xvrt.h"

static const cyaml_schema_field_t xvrt_item_schema[] = {
    CYAML_FIELD_UINT("start",           CYAML_FLAG_DEFAULT, xvrt_item_t, start),
    CYAML_FIELD_UINT("stop",            CYAML_FLAG_DEFAULT, xvrt_item_t, stop),
    CYAML_FIELD_UINT("shift",           CYAML_FLAG_DEFAULT, xvrt_item_t, shift),
    CYAML_FIELD_INT("corr",             CYAML_FLAG_DEFAULT, xvrt_item_t, corr),
    CYAML_FIELD_END
};

const cyaml_schema_value_t xvrt_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_FLOW, xvrt_item_t, xvrt_item_schema)
};
