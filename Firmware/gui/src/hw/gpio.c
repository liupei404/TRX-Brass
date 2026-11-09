/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <gpiod.h>
#include "lvgl/lvgl.h"

#include "gpio.h"
#include "src/settings/options.h"

/* Main unit */

static struct gpiod_line    *line_preamp = NULL;
static struct gpiod_line    *line_tx = NULL;

/* RF unit */

#define BPF_NUM     8
#define LPF_NUM     7
#define ATU_L_NUM   7
#define ATU_C_NUM   7

static struct gpiod_line    *bpf[BPF_NUM];
static struct gpiod_line    *lpf[LPF_NUM];
static struct gpiod_line    *atu_l[ATU_L_NUM];
static struct gpiod_line    *atu_c[ATU_C_NUM];

static struct gpiod_line    *adf_clk = NULL;
static struct gpiod_line    *adf_data = NULL;
static struct gpiod_line    *adf_le = NULL;

static struct gpiod_line    *k39 = NULL;        /* VHF/UHF_RX_TX        */
static struct gpiod_line    *k37 = NULL;        /* XVRT_HF_VHF/UHF      */
static struct gpiod_line    *k38 = NULL;        /* XVRT_VHF/UHF         */
static struct gpiod_line    *k32_k33 = NULL;    /* RELAY_LPF_VHF/UHF    */
static struct gpiod_line    *k34 = NULL;        /* RELAY_VHF_UHF_RX_TX  */
static struct gpiod_line    *k41 = NULL;        /* XVRT_VHF/UHF_RX_TX   */
static struct gpiod_line    *k23 = NULL;        /* RELAY_HF_RX_TX       */
static struct gpiod_line    *k22 = NULL;        /* RELAY_HF_VHF/UHF     */
static struct gpiod_line    *k40 = NULL;        /* SEL_VHF/UHF          */
static struct gpiod_line    *k42 = NULL;        /* TX_430_TX            */
static struct gpiod_line    *rx_144 = NULL;     /* RX_144_RX            */
static struct gpiod_line    *rx_430 = NULL;     /* RX_430_RX            */

static struct gpiod_line    *hf_tx = NULL;

static int16_t              current_bpf = -1;
static int16_t              current_lpf = -1;
static int16_t              current_rf_route = -1;

static struct gpiod_line * init_line(struct gpiod_chip *chip, const char *name) {
    struct gpiod_line * line = gpiod_chip_find_line(chip, name);

    if (line) {
        gpiod_line_request_output(line, "GUI", 0);
    } else {
        LV_LOG_ERROR("Unable to open gpio %s", name);
    }

    return line;
}

static bool set_value(struct gpiod_line *line, uint8_t val) {
    if (line == NULL) {
        return false;
    }

    gpiod_line_set_value(line, val);
    return true;
}

static void main_init() {
    struct gpiod_chip    *chip = NULL;

    chip = gpiod_chip_open_by_number(0);

    if (chip) {
        line_preamp = init_line(chip, "preamp_on");
        line_tx = init_line(chip, "tx_out");
    } else {
        LV_LOG_ERROR("Unable to open gpio chip 0");
    }
}

static void rf_2_init() {
    struct gpiod_chip    *chip = NULL;

    chip = gpiod_chip_open_by_number(2);

    if (chip) {
        const char* bpf_gpio[BPF_NUM] = {
            "bpf_lf", "bpf_160", "bpf_80", "bpf_40",
            "bpf_30_20", "bpf_17_15", "bpf_12_10", "bpf_6"
        };

        for (uint8_t i = 0; i < BPF_NUM; i++)
            bpf[i] = init_line(chip, bpf_gpio[i]);
    } else {
        LV_LOG_ERROR("Unable to open gpio chip 2");
    }
}

static void rf_3_init() {
    struct gpiod_chip    *chip = NULL;

    chip = gpiod_chip_open_by_number(3);

    if (chip) {
        adf_clk = init_line(chip, "adf_clk");
        adf_data = init_line(chip, "adf_data");
        adf_le = init_line(chip, "adf_le");

        k39 = init_line(chip, "k39");
        k37 = init_line(chip, "k37");
        k38 = init_line(chip, "k38");
        k32_k33 = init_line(chip, "k32_k33");
        k34 = init_line(chip, "k34");
        k41 = init_line(chip, "k41");
        k23 = init_line(chip, "k23");
        k22 = init_line(chip, "k22");
        k40 = init_line(chip, "k40");
        k42 = init_line(chip, "k42");
        rx_144 = init_line(chip, "rx_144");
        rx_430 = init_line(chip, "rx_430");
    } else {
        LV_LOG_ERROR("Unable to open gpio chip 3");
    }
}

static void rf_4_init() {
    struct gpiod_chip    *chip = NULL;

    chip = gpiod_chip_open_by_number(4);

    if (chip) {
        const char* lpf_gpio[LPF_NUM] = {
            "lpf_160", "lpf_80", "lpf_40",
            "lpf_20", "lpf_15", "lpf_10", "lpf_6"
        };

        for (uint8_t i = 0; i < LPF_NUM; i++)
            lpf[i] = init_line(chip, lpf_gpio[i]);

        const char* atu_l_gpio[ATU_L_NUM] = {
            "l_01", "l_02", "l_03",
            "l_04", "l_05", "l_06", "l_07"
        };

        for (uint8_t i = 0; i < ATU_L_NUM; i++)
            atu_l[i] = init_line(chip, atu_l_gpio[i]);

        hf_tx = init_line(chip, "hf_tx");
    } else {
        LV_LOG_ERROR("Unable to open gpio chip 4");
    }
}

static void rf_5_init() {
    struct gpiod_chip    *chip = NULL;

    chip = gpiod_chip_open_by_number(5);

    if (chip) {
        const char* atu_c_gpio[ATU_C_NUM] = {
            "c_01", "c_02", "c_03",
            "c_04", "c_05", "c_06", "c_07"
        };

        for (uint8_t i = 0; i < ATU_L_NUM; i++)
            atu_c[i] = init_line(chip, atu_c_gpio[i]);
    } else {
        LV_LOG_ERROR("Unable to open gpio chip 5");
    }
}

void gpio_init() {
    main_init();
    rf_2_init();
    rf_3_init();
    rf_4_init();
    rf_5_init();

    set_value(adf_le, 0); usleep(100);
    set_value(adf_le, 1); usleep(100);
}

void gpio_set_preamp(bool on) {
    if (line_preamp) {
        set_value(line_preamp, on ? 0 : 1);
    }
}

void gpio_set_tx(bool on) {
    switch (current_rf_route) {
        case RF_ROUTE_HF:
            set_value(k22, on ? 1 : 0);
            set_value(hf_tx, on ? 1 : 0);
            break;

        case RF_ROUTE_VHF:
            set_value(k34, on ? 1 : 0);
            set_value(k39, on ? 1 : 0);
            set_value(k41, on ? 1 : 0);
            set_value(rx_144, on ? 0 : 1);
            break;

        case RF_ROUTE_UHF:
            set_value(k34, on ? 1 : 0);
            set_value(k39, on ? 1 : 0);
            set_value(k42, on ? 1 : 0);
            set_value(rx_430, on ? 0 : 1);
            break;
    }

    set_value(line_tx, on ? 1 : 0);
}

void gpio_set_rf_route(rf_route_t route) {
    if (current_rf_route == route)
        return;

    switch (route) {
        case RF_ROUTE_HF:
            set_value(k37, 1);
            set_value(k23, 0);
            set_value(k38, 0);
            set_value(k40, 0);
            break;

        case RF_ROUTE_VHF:
            set_value(k32_k33, 0);
            set_value(k37, 0);
            set_value(k23, 1);
            set_value(k38, 0);
            set_value(k40, 0);
            set_value(rx_144, 1);
            set_value(rx_430, 0);
            break;

        case RF_ROUTE_UHF:
            set_value(k32_k33, 1);
            set_value(k37, 0);
            set_value(k23, 1);
            set_value(k38, 1);
            set_value(k40, 1);
            set_value(rx_144, 0);
            set_value(rx_430, 1);
            break;
    }

    current_rf_route = route;
}

void gpio_set_rf_bpf(uint8_t index) {
    if (current_bpf == index)
        return;

    for (uint8_t i = 0; i < BPF_NUM; i++)
        set_value(bpf[i], i == index ? 1 : 0);

    current_bpf = index;
}

void gpio_set_rf_lpf(uint8_t index) {
    if (current_lpf == index)
        return;

    for (uint8_t i = 0; i < LPF_NUM; i++)
        set_value(lpf[i], i == index ? 1 : 0);

    current_lpf = index;
}

void gpio_send_adf(uint32_t data) {
    if (!set_value(adf_le, 0)) {
        return;
    }

    usleep(10);

    for (int16_t i = 31; i >= 0; i--) {
        uint32_t    mask = (1 << i);
        uint8_t     x = data & mask ? 1 : 0;

        for (int i = 0; i < 10; i++) {
            set_value(adf_data, x);
            usleep(1);
        }

        set_value(adf_clk, 1);
        set_value(adf_clk, 0);
    }

    set_value(adf_le, 1); usleep(10);
}
