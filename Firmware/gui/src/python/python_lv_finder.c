/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_finder.h"
#include "python_lv_object.h"
#include "src/widgets/lv_finder.h"

static int finder_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_finder_create(parent));

    return 0;
}

static PyObject * finder_set_cursor(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    uint8_t index;
    int     hz;

    if (PyArg_ParseTuple(args, "bi", &index, &hz)) {
        lv_finder_set_cursor(self->obj, index, hz);
    }

    Py_RETURN_NONE;
}

static PyMethodDef finder_methods[] = {
    { "set_cursor", (PyCFunction) finder_set_cursor, METH_VARARGS, "" },
    { NULL }
};

PyTypeObject finder_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.finder",
    .tp_doc = PyDoc_STR("LVGL finder"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) finder_init,
    .tp_methods = finder_methods,
};
