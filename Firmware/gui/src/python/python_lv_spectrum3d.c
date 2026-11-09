/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_spectrum3d.h"
#include "python_lv_object.h"
#include "python_lv_grad.h"
#include "src/widgets/lv_spectrum3d.h"

static int spectrum3d_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_spectrum3d_create(parent));

    return 0;
}

static PyObject * spectrum3d_set_data_size(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    int size;
    int depth;

    if (PyArg_ParseTuple(args, "ii", &size, &depth)) {
        lv_spectrum3d_set_data_size(self->obj, size, depth);
    }

    Py_RETURN_NONE;
}

static PyObject * spectrum3d_set_grad(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    PyObject *grad = NULL;

    if (PyArg_ParseTuple(args, "O", &grad)) {
        lv_spectrum3d_set_grad(self->obj, python_lv_get_grad(grad));
    }

    Py_RETURN_NONE;
}

static PyObject * spectrum3d_clear_data(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    lv_spectrum3d_clear_data(self->obj);

    Py_RETURN_NONE;
}

static PyMethodDef spectrum3d_methods[] = {
    { "set_data_size", (PyCFunction) spectrum3d_set_data_size, METH_VARARGS, "" },
    { "set_grad", (PyCFunction) spectrum3d_set_grad, METH_VARARGS, "" },
    { "clear_data", (PyCFunction) spectrum3d_clear_data, METH_NOARGS, "" },
    { NULL }
};

PyTypeObject spectrum3d_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &obj_type,
    .tp_name = "lv.spectrum3d",
    .tp_doc = PyDoc_STR("LVGL spectrum3d"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) spectrum3d_init,
    .tp_methods = spectrum3d_methods,
};
