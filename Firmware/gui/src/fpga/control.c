/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <math.h>
#include <stddef.h>

#include "lvgl/lvgl.h"
#include "control.h"
#include "src/settings/rf.h"

#define RX_ENABLE_ADC  (1 << 0)
#define RX_ENABLE_FFT  (1 << 1)
#define RX_ENABLE_MB   (1 << 2)

typedef struct {
    uint32_t    enable;
    uint32_t    adc_dds_step;
    uint32_t    adc_rate;
    uint32_t    fft_dds_step;
    uint32_t    fft_rate;
    uint32_t    dac_dds_step;
    uint32_t    dac_rate;
} trx_reg_t;

static int          reg_fd;
static trx_reg_t    *reg;

static uint64_t     rx_freq;
static uint64_t     tx_freq;
static uint64_t     fft_freq;

void control_init() {
    /* Reg */

    reg_fd = open("/dev/uio0", O_RDWR | O_SYNC);

    if (reg_fd < 1) {
        LV_LOG_ERROR("Unable to open config device file");
        return;
    }

    reg = (trx_reg_t *) mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, reg_fd, 0);

    if (reg == MAP_FAILED) {
        close(reg_fd);
        LV_LOG_ERROR("Failed to mmap config reg");
    }

    reg->enable = 0;
    usleep(100);
}

void control_update() {
    control_set_rx_freq(rx_freq);
    control_set_tx_freq(tx_freq);
    control_set_fft_freq(fft_freq);
}

void control_set_rx_freq(uint64_t freq) {
    float txo = 122880000.0f + rf->txo_offset;

    rx_freq = freq;
    reg->adc_dds_step = (uint32_t) floor(freq / txo * (1 << 30) + 0.5f);
}

void control_set_rx_rate(uint32_t rate) {
    reg->adc_rate = 122880000 / 8 / rate;
}

void control_set_tx_freq(uint64_t freq) {
    float txo = 122880000.0f + rf->txo_offset;

    tx_freq = freq;
    reg->dac_dds_step = (uint32_t) floor(freq / txo * (1 << 30) + 0.5f);
}

void control_set_fft_freq(uint64_t freq) {
    float txo = 122880000.0f + rf->txo_offset;

    fft_freq = freq;
    reg->fft_dds_step = (uint32_t) floor(freq / txo * (1 << 30) + 0.5f);
}

void control_set_fft_rate(uint32_t rate) {
    reg->fft_rate = rate;
}

void control_fft_enable() {
    reg->enable |= RX_ENABLE_FFT;
}

void control_rx_enable() {
    reg->enable |= RX_ENABLE_ADC;
}

void control_mb_enable() {
    reg->enable |= RX_ENABLE_MB;
}
