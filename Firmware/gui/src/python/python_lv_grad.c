/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_grad.h"

static void grad_dealloc(grad_object_t *self) {
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject * grad_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    grad_object_t *self = (grad_object_t *) type->tp_alloc(type, 0);

    return (PyObject *) self;
}

static PyObject * grad_set_dir(grad_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");

    lv_grad_dir_t value;

    if (PyArg_ParseTuple(args, "b", &value)) {
        self->grad.dir = value;
    }

    Py_RETURN_NONE;
}

static PyObject * grad_set_stops_count(grad_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");

    uint8_t value;

    if (PyArg_ParseTuple(args, "b", &value)) {
        if (value == 0 || value > LV_GRADIENT_MAX_STOPS) {
            PyErr_Format(PyExc_ValueError, "gradient stops count must be between 1 and %d",
                         LV_GRADIENT_MAX_STOPS);
            return NULL;
        }
        self->grad.stops_count = value;
    }

    Py_RETURN_NONE;
}

static PyObject * grad_set_stop(grad_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");

    uint8_t     index;
    uint8_t     frac;
    lv_color_t  color;

    if (PyArg_ParseTuple(args, "bbI", &index, &frac, &color)) {
        if (index >= LV_GRADIENT_MAX_STOPS) {
            PyErr_Format(PyExc_IndexError, "gradient stop index must be less than %d",
                         LV_GRADIENT_MAX_STOPS);
            return NULL;
        }
        self->grad.stops[index].frac = frac;
        self->grad.stops[index].color = color;
        self->grad.stops[index].opa = LV_OPA_COVER;
    }

    Py_RETURN_NONE;
}

static PyMethodDef grad_methods[] = {
    { "set_dir", (PyCFunction) grad_set_dir, METH_VARARGS, "" },
    { "set_stops_count", (PyCFunction) grad_set_stops_count, METH_VARARGS, "" },
    { "set_stop", (PyCFunction) grad_set_stop, METH_VARARGS, "" },
    { NULL }
};

PyTypeObject grad_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lv.grad",
    .tp_doc = PyDoc_STR("LVGL gradient objects"),
    .tp_basicsize = sizeof(grad_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = grad_new,
    .tp_init = NULL,
    .tp_dealloc = (destructor) grad_dealloc,
    .tp_members = NULL,
    .tp_methods = grad_methods,
};

lv_grad_dsc_t * python_lv_get_grad(PyObject *obj) {
    if (PyObject_TypeCheck(obj, &grad_type)) {
        grad_object_t *self = (grad_object_t *) obj;

        return &self->grad;
    }

    return NULL;
}
