/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdbool.h>

typedef enum {
    RF_ROUTE_HF = 0,
    RF_ROUTE_VHF,
    RF_ROUTE_UHF,
} rf_route_t;

void gpio_init();

void gpio_set_preamp(bool on);
void gpio_set_tx(bool on);

void gpio_set_rf_route(rf_route_t route);
void gpio_set_rf_bpf(uint8_t index);
void gpio_set_rf_lpf(uint8_t index);

void gpio_send_adf(uint32_t data);
