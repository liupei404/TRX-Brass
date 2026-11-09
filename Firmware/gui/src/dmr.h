/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <complex.h>

typedef __complex__ float dmr_iq_t;

typedef struct {
    uint32_t    source_id;
    uint32_t    target_id;
    uint32_t    color_code;
    bool        private_call;
    bool        monitor;
} dmr_config_t;

typedef enum {
    DMR_CONFIG_SRC_ID = 0,
    DMR_CONFIG_TARGET_ID,
    DMR_CONFIG_PRIVATE_CALL,
    DMR_CONFIG_COLOR_CODE
} dmr_config_e;

bool dmr_init(void);
void dmr_free(void);
void dmr_reset(void);

void dmr_receive(bool active, const dmr_iq_t *iq, float *pcm, size_t count);
void dmr_tx_set_active(bool active);
void dmr_tx_audio(float sample);
size_t dmr_transmit(dmr_iq_t *iq, size_t capacity);

bool dmr_ptt(bool on, bool dmr_mode);

void dmr_config(dmr_config_e x);
