/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv.h"
#include "python_lv_grad.h"
#include "python_lv_style.h"
#include "python_lv_object.h"
#include "python_lv_hiding.h"
#include "python_lv_label.h"
#include "python_lv_spectrum.h"
#include "python_lv_spectrum3d.h"
#include "python_lv_waterfall.h"
#include "python_lv_finder.h"
#include "python_lv_bandinfo.h"
#include "python_lv_btn.h"
#include "python_lv_xmeter.h"

static PyObject * lv_load_font(PyObject *self, PyObject *args) {
    LV_UNUSED(self);
    const char * path;
    unsigned int font_size;

    if (!PyArg_ParseTuple(args, "sI", &path, &font_size)) return NULL;

    lv_font_t * font = lv_freetype_font_create(path, LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                                font_size, LV_FREETYPE_FONT_STYLE_NORMAL);
    if (font == NULL) {
        PyErr_Format(PyExc_RuntimeError, "unable to load font: %s", path);
        return NULL;
    }

    return PyLong_FromVoidPtr(font);
};

static PyMethodDef lv_methods[] = {
    { "load_font", (PyCFunction) lv_load_font, METH_VARARGS, "" },
    { NULL }
};

static PyModuleDef lv_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "lv",
    .m_doc = "",
    .m_size = -1,
    .m_methods = lv_methods
};

PyMODINIT_FUNC PyInit_lv() {
    PyType_Ready(&grad_type);
    PyType_Ready(&style_type);
    PyType_Ready(&obj_type);
    PyType_Ready(&hiding_type);
    PyType_Ready(&label_type);
    PyType_Ready(&spectrum_type);
    PyType_Ready(&spectrum3d_type);
    PyType_Ready(&waterfall_type);
    PyType_Ready(&finder_type);
    PyType_Ready(&bandinfo_type);
    PyType_Ready(&btn_type);
    PyType_Ready(&xmeter_type);

    PyObject *m = PyModule_Create(&lv_module);

    if (m == NULL) {
        return NULL;
    }

    PyModule_AddObjectRef(m, "grad", (PyObject *) &grad_type);
    PyModule_AddObjectRef(m, "style", (PyObject *) &style_type);
    PyModule_AddObjectRef(m, "obj", (PyObject *) &obj_type);
    PyModule_AddObjectRef(m, "hiding", (PyObject *) &hiding_type);
    PyModule_AddObjectRef(m, "label", (PyObject *) &label_type);
    PyModule_AddObjectRef(m, "spectrum", (PyObject *) &spectrum_type);
    PyModule_AddObjectRef(m, "spectrum3d", (PyObject *) &spectrum3d_type);
    PyModule_AddObjectRef(m, "waterfall", (PyObject *) &waterfall_type);
    PyModule_AddObjectRef(m, "finder", (PyObject *) &finder_type);
    PyModule_AddObjectRef(m, "bandinfo", (PyObject *) &bandinfo_type);
    PyModule_AddObjectRef(m, "btn", (PyObject *) &btn_type);
    PyModule_AddObjectRef(m, "xmeter", (PyObject *) &xmeter_type);

    return m;
}
