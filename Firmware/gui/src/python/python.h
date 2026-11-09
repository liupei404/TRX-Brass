/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <python3.11/Python.h>
#include "lvgl/lvgl.h"

void python_init();
PyObject * python_call(const char *func_name);

lv_obj_t * python_main_screen_get();
lv_style_t * python_get_style(const char *style_name);
lv_grad_dsc_t * python_get_grad(const char *grad_name);
