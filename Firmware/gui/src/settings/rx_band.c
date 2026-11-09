/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "rx_band.h"

static const cyaml_schema_field_t rx_band_item_schema[] = {
    CYAML_FIELD_UINT("to",              CYAML_FLAG_DEFAULT, rx_band_item_t, to),
    CYAML_FIELD_UINT("bpf",             CYAML_FLAG_DEFAULT, rx_band_item_t, bpf),
    CYAML_FIELD_END
};

const cyaml_schema_value_t rx_band_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_FLOW, rx_band_item_t, rx_band_item_schema)
};
