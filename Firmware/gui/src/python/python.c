/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "python.h"
#include "python_lv.h"
#include "python_lv_object.h"
#include "python_lv_style.h"
#include "python_lv_grad.h"
#include "python_trx.h"

static PyObject *module;
static PyObject *py_main_screen;

void python_init() {
    PyObject *name;

    if (PyImport_AppendInittab("lv", PyInit_lv) == -1) {
        LV_LOG_ERROR("LV could not extend in-built modules table");
    }

    if (PyImport_AppendInittab("trx", PyInit_trx) == -1) {
        LV_LOG_ERROR("TRX could not extend in-built modules table");
    }
    
    Py_Initialize();

    name = PyUnicode_DecodeFSDefault("brass");
    module = PyImport_Import(name);
    Py_DECREF(name);
    
    if (module == NULL) {
        PyErr_Print();
        return;
    }
}

PyObject * python_call(const char *func_name) {
    if (module == NULL) {
        return NULL;
    }

    PyObject *func = PyObject_GetAttrString(module, func_name);
    PyObject *value;

    if (func && PyCallable_Check(func)) {
        value = PyObject_CallObject(func, NULL);
    } else {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
    }

    Py_XDECREF(func);

    return value;
}

lv_obj_t * python_main_screen_get() {
    LV_LOG_INFO("begin");
    py_main_screen = python_call("main");
    LV_LOG_INFO("finish");

    if (py_main_screen) {
        return python_lv_get_obj(py_main_screen);
    } else {
        LV_LOG_ERROR("Main screen object not found");
    }

    return NULL;
}

lv_style_t * python_get_style(const char *style_name) {
    if (module == NULL) {
        return NULL;
    }

    PyObject *style = PyObject_GetAttrString(module, style_name);
    lv_style_t *value = NULL;

    if (style) {
        value = python_lv_get_style(style);
    } else {
        LV_LOG_ERROR("Style %s not found", style_name);
    }

    Py_XDECREF(style);

    return value;
}

lv_grad_dsc_t * python_get_grad(const char *grad_name) {
    if (module == NULL) {
        return NULL;
    }

    PyObject *grad = PyObject_GetAttrString(module, grad_name);
    lv_grad_dsc_t *value = NULL;

    if (grad) {
        value = python_lv_get_grad(grad);
    } else {
        LV_LOG_ERROR("Gradient %s not found", grad_name);
    }

    Py_XDECREF(grad);

    return value;
}
