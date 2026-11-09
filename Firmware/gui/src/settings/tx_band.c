/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "tx_band.h"

static const cyaml_schema_field_t tx_band_item_schema[] = {
    CYAML_FIELD_UINT("to",              CYAML_FLAG_DEFAULT, tx_band_item_t, to),
    CYAML_FIELD_UINT("lpf",             CYAML_FLAG_DEFAULT, tx_band_item_t, lpf),
    CYAML_FIELD_UINT("vref",            CYAML_FLAG_DEFAULT, tx_band_item_t, vref),
    CYAML_FIELD_END
};

const cyaml_schema_value_t tx_band_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_FLOW, tx_band_item_t, tx_band_item_schema)
};
