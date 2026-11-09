/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include "brass_msg.h"
#include "widgets/lv_finder.h"

typedef struct {
    float   min;
    float   max;
} msgs_auto_t;

typedef struct {
    float   *data;
    size_t  size;
} msgs_floats_t;

typedef struct {
    uint8_t num;
    int16_t cursor[LV_FINDER_CURSORS];
} msg_finder_cursor_t;

typedef enum {
    MSG_MODE_CHANGED = 1,       /* op_work->mode */
    MSG_FILTER_CHANGED,
    MSG_BAND_CHANGED,
    MSG_FREQ_RX_PRE_CHANGED,    /* uint64_t */
    MSG_FREQ_RX_CHANGED,        /* uint64_t */
    MSG_FREQ_TX_PRE_CHANGED,    /* uint64_t */
    MSG_FREQ_TX_CHANGED,        /* uint64_t */
    MSG_FREQ_FFT_CHANGED,       /* uint64_t */
    MSG_FREQ_FFT_SHIFT,         /* int32_t */
    MSG_RATE_FFT_CHANGED,       /* uint8_t */
    MSG_SPECTRUM_AUTO,          /* msgs_auto_t */
    MSG_WATERFALL_AUTO,         /* msgs_auto_t */
    MSG_SPECTRUM_DATA,          /* msgs_floats_t */
    MSG_WATERFALL_DATA,         /* msgs_floats_t */
    MSG_PTT,                    /* int32_t */
    MSG_TX,
    MSG_RX,
    MSG_FINDER_CURSOR,
    MSG_RECORDER,               /* bool */
    MSG_MSG,                    /* char */
    MSG_MSG_LONG,               /* char */
    MSG_MSG_TINY,               /* char */
    MSG_SMETER,                 /* float */
    MSG_AGC_CHANGED,            /* op_mode->agc */
    MSG_ANT_CHANGED,            /* rf->ant */
    MSG_SPLIT_CHANGED,          /* op_work->split */
    MSG_MIC_METER,              /* float */
    MSG_MIC,                    /* bool */
    MSG_SWR_METER,              /* float */
    MSG_PWR_METER,              /* float */
} msgs_t;
