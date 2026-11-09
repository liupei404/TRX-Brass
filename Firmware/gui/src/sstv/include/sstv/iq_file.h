/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    SSTV_IQ_READ_OK,
    SSTV_IQ_READ_EOF,
    SSTV_IQ_READ_TRUNCATED,
    SSTV_IQ_READ_ERROR
} sstv_iq_read_status_t;

sstv_iq_read_status_t sstv_iq_read(FILE *file, float complex *samples, size_t capacity, size_t *count);

