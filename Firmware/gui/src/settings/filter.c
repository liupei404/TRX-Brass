/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "filter.h"

const cyaml_schema_field_t filter_fields_schema[] = {
    CYAML_FIELD_UINT("low",             CYAML_FLAG_DEFAULT, filter_t, low),
    CYAML_FIELD_UINT("high",            CYAML_FLAG_DEFAULT, filter_t, high),
    CYAML_FIELD_UINT("transition",      CYAML_FLAG_DEFAULT, filter_t, transition),
    CYAML_FIELD_END
};
