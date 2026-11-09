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

typedef struct obj_subscription_t obj_subscription_t;

typedef struct {
    PyObject_HEAD
    lv_obj_t              *obj;
    obj_subscription_t    *subscriptions;
} obj_object_t;

extern PyTypeObject obj_type;

lv_obj_t * python_lv_get_obj(PyObject *obj);
void python_lv_set_obj(obj_object_t *self, lv_obj_t *obj);

#define PYTHON_LV_REQUIRE_OBJ(self) \
    do { \
        if ((self)->obj == NULL) { \
            PyErr_SetString(PyExc_RuntimeError, "LVGL object has been deleted"); \
            return NULL; \
        } \
    } while (0)
