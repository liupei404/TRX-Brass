/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_bandinfo.h"
#include "python_lv_object.h"
#include "python_lv_hiding.h"
#include "src/widgets/lv_bandinfo.h"

static int bandinfo_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_bandinfo_create(parent));

    return 0;
}

static PyObject * bandinfo_set_span(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    int32_t span;

    if (PyArg_ParseTuple(args, "i", &span)) {
        lv_bandinfo_set_span(self->obj, span);
    }

    Py_RETURN_NONE;
}

static PyObject * bandinfo_set_center(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    uint64_t center;

    if (PyArg_ParseTuple(args, "K", &center)) {
        lv_bandinfo_set_center(self->obj, center);
    }

    Py_RETURN_NONE;
}


static PyMethodDef bandinfo_methods[] = {
    { "set_span", (PyCFunction) bandinfo_set_span, METH_VARARGS, "" },
    { "set_center", (PyCFunction) bandinfo_set_center, METH_VARARGS, "" },
    { NULL }
};

PyTypeObject bandinfo_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &hiding_type,
    .tp_name = "lv.bandinfo",
    .tp_doc = PyDoc_STR("LVGL band info"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) bandinfo_init,
    .tp_methods = bandinfo_methods,
};
