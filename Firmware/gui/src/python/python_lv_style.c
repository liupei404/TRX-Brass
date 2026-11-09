/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_style.h"
#include "python_lv_grad.h"

static void style_dealloc(style_object_t *self) {
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject * style_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    style_object_t *self = (style_object_t *) type->tp_alloc(type, 0);

    if (self != NULL) {
        lv_style_init(&self->style);
    }

    return (PyObject *) self;
}

static PyObject * style_set_bg(style_object_t *self, PyObject *args, PyObject *kwargs) {
    long int        color = -1L;
    long int        opa = -1L;
    char            *img_src = NULL;
    long int        img_opa = -1L;
    PyObject        *grad = NULL;
    long int        grad_color = -1L;
    long int        grad_dir = -1L;
    long int        main_stop = -1L;
    long int        grad_stop = -1L;
    char            *kwlist[] = {"color", "opa", "img_src", "img_opa", "grad", "grad_color", "grad_dir", "main_stop", "grad_stop", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$llslOllll", kwlist, &color, &opa, &img_src, &img_opa, &grad, &grad_color, &grad_dir, &main_stop, &grad_stop)) {
        return NULL;
    }

    if (color != -1)        lv_style_set_bg_color(&self->style, lv_color_hex(color));
    if (opa != -1)          lv_style_set_bg_opa(&self->style, (lv_opa_t) opa);
    if (img_src != NULL)    lv_style_set_bg_image_src(&self->style, strdup(img_src));
    if (img_opa != -1)      lv_style_set_bg_image_opa(&self->style, (lv_opa_t) img_opa);
    if (grad_color != -1)   lv_style_set_bg_grad_color(&self->style, lv_color_hex(grad_color));
    if (grad_dir != -1)     lv_style_set_bg_grad_dir(&self->style, (lv_grad_dir_t) grad_dir);
    if (main_stop != -1)    lv_style_set_bg_main_stop(&self->style, main_stop);
    if (grad_stop != -1)    lv_style_set_bg_grad_stop(&self->style, grad_stop);
    if (grad != NULL)       lv_style_set_bg_grad(&self->style, python_lv_get_grad(grad));

    Py_RETURN_NONE;
}

static PyObject * style_set_border(style_object_t *self, PyObject *args, PyObject *kwargs) {
    long int    color = -1L;
    long int    opa = -1L;
    long int    width = -1L;
    char        *kwlist[] = {"color", "opa", "width", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$lll", kwlist, &color, &opa, &width)) {
        return NULL;
    }

    if (color != -1)    lv_style_set_border_color(&self->style, lv_color_hex(color));
    if (opa != -1)      lv_style_set_border_opa(&self->style, (lv_opa_t) opa);
    if (width != -1)    lv_style_set_border_width(&self->style, (lv_coord_t) width);

    Py_RETURN_NONE;
}

static PyObject * style_set_radius(style_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");

    lv_coord_t radius;

    if (PyArg_ParseTuple(args, "i", &radius)) {
        lv_style_set_radius(&self->style, radius);
    }

    Py_RETURN_NONE;
}

static PyObject * style_set_geom(style_object_t *self, PyObject *args, PyObject *kwargs) {
    lv_coord_t  x = -1;
    lv_coord_t  y = -1;
    lv_coord_t  width = -1;
    lv_coord_t  height = -1;
    char        *kwlist[] = { "x", "y", "width", "height", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$iiii", kwlist, &x, &y, &width, &height)) {
        return NULL;
    }

    if (x != -1)        lv_style_set_x(&self->style, x);
    if (y != -1)        lv_style_set_y(&self->style, y);
    if (width != -1)    lv_style_set_width(&self->style, width);
    if (height != -1)   lv_style_set_height(&self->style, height);

    Py_RETURN_NONE;
}

static PyObject * style_set_pad(style_object_t *self, PyObject *args, PyObject *kwargs) {
    lv_coord_t  all = -1;
    lv_coord_t  hor = -1;
    lv_coord_t  ver = -1;
    lv_coord_t  left = -1;
    lv_coord_t  right = -1;
    lv_coord_t  top = -1;
    lv_coord_t  bottom = -1;
    lv_coord_t  column = -1;
    lv_coord_t  row = -1;
    char        *kwlist[] = { "all", "hor", "ver", "left", "right", "top", "bottom", "column", "row", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$iiiiiiiii", kwlist, &all, &hor, &ver, &left, &right, &top, &bottom, &column, &row)) {
        return NULL;
    }

    if (all != -1)      lv_style_set_pad_all(&self->style, all);
    if (hor != -1)      lv_style_set_pad_hor(&self->style, hor);
    if (ver != -1)      lv_style_set_pad_ver(&self->style, ver);
    if (left != -1)     lv_style_set_pad_left(&self->style, left);
    if (right != -1)    lv_style_set_pad_right(&self->style, right);
    if (top != -1)      lv_style_set_pad_top(&self->style, top);
    if (bottom != -1)   lv_style_set_pad_bottom(&self->style, bottom);
    if (column != -1)   lv_style_set_pad_column(&self->style, column);
    if (row != -1)      lv_style_set_pad_row(&self->style, row);

    Py_RETURN_NONE;
}

static PyObject * style_set_line(style_object_t *self, PyObject *args, PyObject *kwargs) {
    long int        width = -1L;
    long int        color = -1L;
    long int        opa = -1L;

    char            *kwlist[] = { "width", "color", "opa", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$lll", kwlist, &width, &color, &opa)) {
        return NULL;
    }

    if (width != -1)    lv_style_set_line_width(&self->style, (lv_coord_t) width);
    if (color != -1)    lv_style_set_line_color(&self->style, lv_color_hex(color));
    if (opa != -1)      lv_style_set_line_opa(&self->style, (lv_opa_t) opa);

    Py_RETURN_NONE;
}

static PyObject * style_set_text(style_object_t *self, PyObject *args, PyObject *kwargs) {
    long int        color = -1;
    long int        align = -1;
    void            *font = NULL;
    char            *kwlist[] = { "color", "align", "font", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$llk", kwlist, &color, &align, &font)) {
        return NULL;
    }

    if (color != -1)    lv_style_set_text_color(&self->style, lv_color_hex(color));
    if (align != -1)    lv_style_set_text_align(&self->style, (lv_text_align_t) align);
    if (font != NULL)   lv_style_set_text_font(&self->style, font);

    Py_RETURN_NONE;
}

static PyObject * style_set_max(style_object_t *self, PyObject *args, PyObject *kwargs) {
    long int        width = -1;
    long int        height = -1;
    char            *kwlist[] = { "width", "height",  NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$ll", kwlist, &width, &height)) {
        return NULL;
    }

    if (width != -1)    lv_style_set_max_width(&self->style, (lv_coord_t) width);
    if (height != -1)   lv_style_set_max_height(&self->style, (lv_coord_t) height);

    Py_RETURN_NONE;
}

static PyMethodDef style_methods[] = {
    { "set_bg", (PyCFunction) style_set_bg, METH_VARARGS | METH_KEYWORDS, "" },
    { "set_border", (PyCFunction) style_set_border, METH_VARARGS | METH_KEYWORDS, "" },
    { "set_radius", (PyCFunction) style_set_radius, METH_VARARGS, "" },
    { "set_geom", (PyCFunction) style_set_geom, METH_VARARGS | METH_KEYWORDS, "" },
    { "set_pad", (PyCFunction) style_set_pad, METH_VARARGS | METH_KEYWORDS, "" },
    { "set_line", (PyCFunction) style_set_line, METH_VARARGS | METH_KEYWORDS, "" },
    { "set_text", (PyCFunction) style_set_text, METH_VARARGS | METH_KEYWORDS, "" },
    { "set_max", (PyCFunction) style_set_max, METH_VARARGS | METH_KEYWORDS, "" },
    { NULL }
};

PyTypeObject style_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lv.style",
    .tp_doc = PyDoc_STR("LVGL style objects"),
    .tp_basicsize = sizeof(style_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = style_new,
    .tp_init = NULL,
    .tp_dealloc = (destructor) style_dealloc,
    .tp_members = NULL,
    .tp_methods = style_methods,
};

lv_style_t * python_lv_get_style(PyObject *obj) {
    if (PyObject_TypeCheck(obj, &style_type)) {
        style_object_t *self = (style_object_t *) obj;

        return &self->style;
    }

    return NULL;
}
