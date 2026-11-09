/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include "op_work.h"

typedef struct {
    const char      *label;
    op_work_t       work;
} memory_item_t;

typedef struct {
    memory_item_t   *item;
    unsigned        count;
} memory_t;

memory_t * settings_memory_load(const char *filename);
void settings_memory_save(const char *filename, memory_t *memory);
void settings_memory_free(memory_t *memory);
