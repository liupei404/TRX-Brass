/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_label.h"
#include "python_lv_object.h"

static int label_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_label_create(parent));

    return 0;
}

static PyObject * label_set_text(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    const char *text;

    if (PyArg_ParseTuple(args, "s", &text)) {
        lv_label_set_text(self->obj, text);
    }

    Py_RETURN_NONE;
}

static PyMethodDef label_methods[] = {
    { "set_text", (PyCFunction) label_set_text, METH_VARARGS, "" },
    { NULL }
};

PyTypeObject label_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.label",
    .tp_doc = PyDoc_STR("LVGL label"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) label_init,
    .tp_methods = label_methods,
};
