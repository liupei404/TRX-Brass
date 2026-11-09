/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include "lvgl/lvgl.h"

void queue_init();
void queue_send(lv_obj_t *obj, lv_event_code_t event_code, void *param);
void queue_cancel(lv_obj_t *obj, lv_event_code_t event_code, void (*free_param)(void *));
void queue_work();
