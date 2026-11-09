/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_btn.h"
#include "python_lv_object.h"

static int btn_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_btn_create(parent));

    return 0;
}

static PyMethodDef btn_methods[] = {
    { NULL }
};

PyTypeObject btn_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.btn",
    .tp_doc = PyDoc_STR("LVGL button"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) btn_init,
    .tp_methods = btn_methods,
};
