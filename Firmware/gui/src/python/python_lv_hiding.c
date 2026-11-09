/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_hiding.h"
#include "python_lv_object.h"
#include "src/widgets/lv_hiding.h"

static int hiding_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_hiding_create(parent));

    return 0;
}

static PyObject * hiding_set_timeout(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    uint16_t timeout;

    if (PyArg_ParseTuple(args, "H", &timeout)) {
        lv_hiding_set_timeout(self->obj, timeout);
    }

    Py_RETURN_NONE;
}

static PyObject * hiding_touch(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    lv_hiding_touch(self->obj);

    Py_RETURN_NONE;
}

static PyMethodDef hiding_methods[] = {
    { "set_timeout", (PyCFunction) hiding_set_timeout, METH_VARARGS, "" },
    { "touch", (PyCFunction) hiding_touch, METH_NOARGS, "" },
    { NULL }
};

PyTypeObject hiding_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.hiding",
    .tp_doc = PyDoc_STR("LVGL hiding object"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) hiding_init,
    .tp_methods = hiding_methods,
};
