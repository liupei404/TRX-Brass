/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "memory.h"
#include "radio.h"
#include "msg.h"

const char * memory_load(const memory_t *memory, uint32_t index) {
    if (index > memory->count - 1) {
        return NULL;
    }

    memory_item_t   *item = &memory->item[index];

    op_work = &item->work;
    op_work_activate(true);

    if (item->label != NULL) {
        msg_set_text_fmt("%s", item->label);
    }

    return item->label;
}

void memory_save(const memory_t *memory, uint32_t index) {
}
