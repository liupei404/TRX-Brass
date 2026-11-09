/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

static inline size_t Exp2(uint32_t X) {
    return (uint32_t) 1 << X;
}

static inline size_t Log2(uint32_t X) {
    uint32_t Y;

    for (Y = 0; X > 1; X >>= 1) {
        Y += 1;
    }
    return Y;
}
