/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include "settings/memory.h"

const char * memory_load(const memory_t *memory, uint32_t index);
void memory_save(const memory_t *memory, uint32_t index);
