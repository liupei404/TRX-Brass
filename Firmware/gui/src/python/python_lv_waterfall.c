/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_waterfall.h"
#include "python_lv_object.h"
#include "python_lv_grad.h"
#include "src/widgets/lv_waterfall.h"

static int waterfall_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_waterfall_create(parent));

    return 0;
}

static PyObject * waterfall_set_data_size(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    int w;

    if (PyArg_ParseTuple(args, "i", &w)) {
        lv_waterfall_set_data_size(self->obj, w);
    }

    Py_RETURN_NONE;
}

static PyObject * waterfall_clear_data(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    lv_waterfall_clear_data(self->obj);

    Py_RETURN_NONE;
}

static PyObject * waterfall_set_grad(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    PyObject *grad = NULL;

    if (PyArg_ParseTuple(args, "O", &grad)) {
        lv_waterfall_set_grad(self->obj, python_lv_get_grad(grad));
    }

    Py_RETURN_NONE;
}

static PyMethodDef waterfall_methods[] = {
    { "set_data_size", (PyCFunction) waterfall_set_data_size, METH_VARARGS, "" },
    { "set_grad", (PyCFunction) waterfall_set_grad, METH_VARARGS, "" },
    { "clear_data", (PyCFunction) waterfall_clear_data, METH_NOARGS, "" },
    { NULL }
};

PyTypeObject waterfall_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.waterfall",
    .tp_doc = PyDoc_STR("LVGL waterfall"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) waterfall_init,
    .tp_methods = waterfall_methods,
};
