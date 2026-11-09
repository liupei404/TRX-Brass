/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_spectrum.h"
#include "python_lv_object.h"
#include "src/widgets/lv_spectrum.h"

static int spectrum_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_spectrum_create(parent));

    return 0;
}

static PyObject * spectrum_set_data_size(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    int size;

    if (PyArg_ParseTuple(args, "i", &size)) {
        lv_spectrum_set_data_size(self->obj, size);
    }

    Py_RETURN_NONE;
}

static PyObject * spectrum_clear_data(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    lv_spectrum_clear_data(self->obj);

    Py_RETURN_NONE;
}

static PyObject * spectrum_set_peak(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    int on;

    if (PyArg_ParseTuple(args, "b", &on)) {
        lv_spectrum_set_peak(self->obj, on);
    }

    Py_RETURN_NONE;
}

static PyObject * spectrum_set_peak_hold(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    int ms;

    if (PyArg_ParseTuple(args, "i", &ms)) {
        lv_spectrum_set_peak_hold(self->obj, ms);
    }

    Py_RETURN_NONE;
}

static PyObject * spectrum_set_peak_speed(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    float db;

    if (PyArg_ParseTuple(args, "f", &db)) {
        lv_spectrum_set_peak_speed(self->obj, db);
    }

    Py_RETURN_NONE;
}

static PyMethodDef spectrum_methods[] = {
    { "set_data_size", (PyCFunction) spectrum_set_data_size, METH_VARARGS, "" },
    { "clear_data", (PyCFunction) spectrum_clear_data, METH_NOARGS, "" },
    { "set_peak", (PyCFunction) spectrum_set_peak, METH_VARARGS, "" },
    { "set_peak_hold", (PyCFunction) spectrum_set_peak_hold, METH_VARARGS, "" },
    { "set_peak_speed", (PyCFunction) spectrum_set_peak_speed, METH_VARARGS, "" },
    { NULL }
};

PyTypeObject spectrum_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.spectrum",
    .tp_doc = PyDoc_STR("LVGL spectrum"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) spectrum_init,
    .tp_methods = spectrum_methods,
};
