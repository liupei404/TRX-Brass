/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>

template <class Type> Type GrayCode(Type Binary) {
    return Binary ^ (Binary >> 1);
}

inline uint8_t BinaryCode(uint8_t Gray) {
    Gray ^= (Gray >> 4);
    Gray ^= (Gray >> 2);
    Gray ^= (Gray >> 1);
    return Gray;
}

inline uint16_t BinaryCode(uint16_t Gray) {
    Gray ^= (Gray >> 8);
    Gray ^= (Gray >> 4);
    Gray ^= (Gray >> 2);
    Gray ^= (Gray >> 1);
    return Gray;
}

inline uint32_t BinaryCode(uint32_t Gray) {
    Gray ^= (Gray >> 16);
    Gray ^= (Gray >> 8);
    Gray ^= (Gray >> 4);
    Gray ^= (Gray >> 2);
    Gray ^= (Gray >> 1);
    return Gray;
}
