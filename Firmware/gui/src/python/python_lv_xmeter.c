/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_xmeter.h"
#include "python_lv_object.h"
#include "src/widgets/lv_xmeter.h"

static int xmeter_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_xmeter_create(parent));

    return 0;
}

static PyObject * xmeter_set_value(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    float value;

    if (PyArg_ParseTuple(args, "f", &value)) {
        lv_xmeter_set_value(self->obj, value);
    }

    Py_RETURN_NONE;
}

static PyObject * xmeter_set_slice(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    float value;

    if (PyArg_ParseTuple(args, "f", &value)) {
        lv_xmeter_set_slice(self->obj, value);
    }

    Py_RETURN_NONE;
}

static PyObject * xmeter_set_part(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    uint8_t index;
    float   value;

    if (PyArg_ParseTuple(args, "bf", &index, &value)) {
        lv_xmeter_set_part(self->obj, index, value);
    }

    Py_RETURN_NONE;
}

static PyObject * xmeter_set_label(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    uint8_t index;
    char    *text;
    float   value;

    if (PyArg_ParseTuple(args, "bsf", &index, &text, &value)) {
        lv_xmeter_set_label(self->obj, index, text, value);
    }

    Py_RETURN_NONE;
}

static PyObject * xmeter_set_range(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    float   min;
    float   max;

    if (PyArg_ParseTuple(args, "ff", &min, &max)) {
        lv_xmeter_set_range(self->obj, min, max);
    }

    Py_RETURN_NONE;
}

static PyMethodDef xmeter_methods[] = {
    { "set_value", (PyCFunction) xmeter_set_value, METH_VARARGS, "" },
    { "set_slice", (PyCFunction) xmeter_set_slice, METH_VARARGS, "" },
    { "set_part", (PyCFunction) xmeter_set_part, METH_VARARGS, "" },
    { "set_label", (PyCFunction) xmeter_set_label, METH_VARARGS, "" },
    { "set_range", (PyCFunction) xmeter_set_range, METH_VARARGS, "" },
    { NULL }
};

PyTypeObject xmeter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.xmeter",
    .tp_doc = PyDoc_STR("LVGL X-Meter"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) xmeter_init,
    .tp_methods = xmeter_methods,
};
