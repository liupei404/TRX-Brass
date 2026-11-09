/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 *
 *  Based on Libitree: an interval tree library in C
 *  Copyright (C) 2018 Alessandro Vullo
 */

#include "interval.h"

#include <stdio.h>
#include <stdlib.h>

int band_overlap(const band_t *b1, const uint64_t start, const uint64_t stop) {
    return b1->settings.start <= stop && start <= b1->settings.stop;
}

int band_equal(const band_t *b1, const band_t *b2) {
    return b1->settings.start == b2->settings.start && b1->settings.stop == b2->settings.stop;
}
