/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <python3.11/Python.h>

#include "lvgl/lvgl.h"

typedef struct {
    PyObject_HEAD
    lv_style_t  style;
} style_object_t;

extern PyTypeObject style_type;

lv_style_t * python_lv_get_style(PyObject *obj);
