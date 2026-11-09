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
#include <stdint.h>

#include "sstv/modes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returning false cancels encoding immediately. The sink is not called again,
 * and sstv_encode_rgb() returns false. */
typedef bool (*sstv_iq_sink_t)(void *user, const float complex *samples, size_t count);

/* Encodes an RGB888 image synchronously. Image dimensions must equal
 * mode->width by mode->transmitted_lines. Stride is measured in bytes. */
bool sstv_encode_rgb(const sstv_mode_t *mode, const uint8_t *rgb, size_t stride, sstv_iq_sink_t sink, void *user);

#ifdef __cplusplus
}
#endif
