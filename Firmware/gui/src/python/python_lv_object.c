/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "python_lv_object.h"
#include "python_lv_style.h"
#include "src/msgs.h"

typedef struct {
    lv_timer_t      *timer;
    obj_object_t    *owner;
    PyObject        *call;
} obj_timer_t;

struct obj_subscription_t {
    brass_msg_subscription_t   *subscription;
    PyObject                   *call;
    obj_subscription_t         *next;
};

static void obj_clear_subscriptions(obj_object_t *self) {
    obj_subscription_t *item = self->subscriptions;

    self->subscriptions = NULL;
    while (item != NULL) {
        obj_subscription_t *next = item->next;

        brass_msg_unsubscribe(item->subscription);
        Py_DECREF(item->call);
        PyMem_Free(item);
        item = next;
    }
}

static void obj_delete_cb(lv_event_t *event) {
    obj_object_t *self = lv_event_get_user_data(event);

    self->obj = NULL;
    obj_clear_subscriptions(self);
}

void python_lv_set_obj(obj_object_t *self, lv_obj_t *obj) {
    self->obj = obj;
    if (obj != NULL) {
        lv_obj_add_event_cb(obj, obj_delete_cb, LV_EVENT_DELETE, self);
    }
}

static void obj_dealloc(obj_object_t *self) {
    if (self->obj != NULL) {
        lv_obj_delete(self->obj);
        self->obj = NULL;
    }

    obj_clear_subscriptions(self);

    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject * obj_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    obj_object_t *self = (obj_object_t *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->obj = NULL;
        self->subscriptions = NULL;
    }

    return (PyObject *) self;
}

static int obj_init(obj_object_t *self, PyObject *args, PyObject *kwds) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;
    lv_obj_t    *parent = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        parent = python_lv_get_obj(obj);
    }

    python_lv_set_obj(self, lv_obj_create(parent));

    return 0;
}

static void obj_msg_cb(void *s, brass_msg_t *m) {
    PyObject    *call = brass_msg_get_user_data(m);
    PyObject    *arg;
    uint32_t    msg = brass_msg_get_id(m);

    switch (msg) {
        case MSG_FREQ_FFT_CHANGED:
        case MSG_FREQ_TX_CHANGED:
        case MSG_FREQ_RX_CHANGED: {
            const uint64_t *x = brass_msg_get_payload(m);

            arg = Py_BuildValue("IL", msg, *x);
        } break;

        case MSG_MIC:
        case MSG_RECORDER: {
            const bool *x = brass_msg_get_payload(m);

            arg = Py_BuildValue("Ib", msg, *x);
        } break;

        case MSG_MSG:
        case MSG_MSG_LONG:
        case MSG_MSG_TINY: {
            const char *x = brass_msg_get_payload(m);

            arg = Py_BuildValue("Is", msg, x);
        } break;

        case MSG_RATE_FFT_CHANGED:
        case MSG_AGC_CHANGED:
        case MSG_ANT_CHANGED:
        case MSG_SPLIT_CHANGED:
        case MSG_MODE_CHANGED: {
            const uint8_t *x = brass_msg_get_payload(m);

            arg = Py_BuildValue("Ib", msg, *x);
        } break;

        default:
            arg = Py_BuildValue("IO", msg, Py_None);
            break;
    }

    Py_INCREF(call);
    PyObject *res = PyObject_Call(call, arg, NULL);

    Py_XDECREF(arg);

    if (res) {
        Py_XDECREF(res);
    } else {
        PyErr_Print();
    }

    Py_DECREF(call);
}

static PyObject * obj_msg_subscribe(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    uint32_t    msg_id;
    PyObject    *obj = NULL;

    if (!PyArg_ParseTuple(args, "iO", &msg_id, &obj)) {
        return NULL;
    }

    if (!PyCallable_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "message callback must be callable");
        return NULL;
    }

    obj_subscription_t *item = PyMem_Malloc(sizeof(*item));
    if (item == NULL) {
        return PyErr_NoMemory();
    }

    Py_INCREF(obj);
    item->call = obj;
    item->subscription = brass_msg_subscribe(msg_id, obj_msg_cb, obj);
    if (item->subscription == NULL) {
        Py_DECREF(obj);
        PyMem_Free(item);
        return PyErr_NoMemory();
    }

    item->next = self->subscriptions;
    self->subscriptions = item;

    Py_RETURN_NONE;
}

static void obj_timer_delete_cb(lv_event_t *event) {
    obj_timer_t *timer = lv_event_get_user_data(event);

    /* The LVGL object can be deleted by its parent while its Python wrapper
     * is still alive.  Mark it invalid before releasing the bound method:
     * its DECREF can immediately run obj_dealloc(). */
    timer->owner->obj = NULL;

    if (timer->timer != NULL) {
        lv_timer_delete(timer->timer);
        timer->timer = NULL;
    }

    Py_DECREF(timer->call);
    lv_free(timer);
}

static void obj_timer_cb(lv_timer_t *lv_timer) {
    obj_timer_t *timer = lv_timer_get_user_data(lv_timer);
    PyObject    *call = timer->call;

    /* The callback is allowed to delete its owner and therefore the timer
     * context. Keep the callable alive until PyObject_CallNoArgs returns. */
    Py_INCREF(call);
    PyObject    *res = PyObject_CallNoArgs(call);

    if (res != NULL) {
        Py_DECREF(res);
    } else {
        PyErr_Print();
    }

    Py_DECREF(call);
}

static PyObject * obj_timer_create(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");

    uint32_t    period;
    PyObject    *call = NULL;

    if (!PyArg_ParseTuple(args, "OI", &call, &period)) {
        return NULL;
    }

    if (!PyCallable_Check(call)) {
        PyErr_SetString(PyExc_TypeError, "timer callback must be callable");
        return NULL;
    }

    if (self->obj == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "cannot create timer for deleted LVGL object");
        return NULL;
    }

    obj_timer_t *timer = lv_malloc(sizeof(*timer));

    if (timer == NULL) {
        return PyErr_NoMemory();
    }

    Py_INCREF(call);
    timer->owner = self;
    timer->call = call;
    timer->timer = lv_timer_create(obj_timer_cb, period, timer);

    if (timer->timer == NULL) {
        Py_DECREF(call);
        lv_free(timer);
        return PyErr_NoMemory();
    }

    lv_obj_add_event_cb(self->obj, obj_timer_delete_cb, LV_EVENT_DELETE, timer);

    Py_RETURN_NONE;
}

static PyObject * obj_remove_style_all(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    lv_obj_remove_style_all(self->obj);

    Py_RETURN_NONE;
}

static PyObject * obj_add_style(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    PyObject    *obj = NULL;
    lv_part_t   selector;

    if (PyArg_ParseTuple(args, "Ol", &obj, &selector)) {
        if (PyObject_TypeCheck(obj, &style_type)) {
            Py_INCREF(obj);

            style_object_t *style = (style_object_t *) obj;

            lv_obj_add_style(self->obj, &style->style, selector);
        }
    }

    Py_RETURN_NONE;
}

static PyObject * obj_clear_flag(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    lv_obj_flag_t flag;

    if (PyArg_ParseTuple(args, "i", &flag)) {
        lv_obj_clear_flag(self->obj, flag);
    }

    Py_RETURN_NONE;
}

static PyObject * obj_add_flag(obj_object_t *self, PyObject *args) {
    LV_LOG_INFO("begin");
    PYTHON_LV_REQUIRE_OBJ(self);

    lv_obj_flag_t flag;

    if (PyArg_ParseTuple(args, "i", &flag)) {
        lv_obj_add_flag(self->obj, flag);
    }

    Py_RETURN_NONE;
}

static PyObject * obj_set_pos(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    lv_coord_t x, y;

    if (PyArg_ParseTuple(args, "ii", &x, &y)) {
        lv_obj_set_pos(self->obj, x, y);
    }

    Py_RETURN_NONE;
}

static PyObject * obj_set_size(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    lv_coord_t w, h;

    if (PyArg_ParseTuple(args, "ii", &w, &h)) {
        lv_obj_set_size(self->obj, w, h);
    }

    Py_RETURN_NONE;
}

static PyObject * obj_set_style_line_width(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    LV_LOG_INFO("begin");

    lv_coord_t          width;
    lv_style_selector_t selector;

    if (PyArg_ParseTuple(args, "il", &width, &selector)) {
        lv_obj_set_style_line_width(self->obj, width, selector);
    }

    Py_RETURN_NONE;
}

static PyObject * obj_set_style_line_color(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    LV_LOG_INFO("begin");

    lv_color_t          color;
    lv_style_selector_t selector;

    if (PyArg_ParseTuple(args, "Ll", &color, &selector)) {
        lv_obj_set_style_line_color(self->obj, color, selector);
    }

    Py_RETURN_NONE;
}

static PyObject * obj_set_style_opa(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    LV_LOG_INFO("begin");

    lv_opa_t            opa;
    lv_style_selector_t selector;

    if (PyArg_ParseTuple(args, "bl", &opa, &selector)) {
        lv_obj_set_style_opa(self->obj, opa, selector);
    }

    Py_RETURN_NONE;
}

static PyObject * obj_move_foreground(obj_object_t *self, PyObject *args) {
    PYTHON_LV_REQUIRE_OBJ(self);
    lv_obj_move_foreground(self->obj);

    Py_RETURN_NONE;
}

static PyMethodDef obj_methods[] = {
    { "msg_subscribe", (PyCFunction) obj_msg_subscribe, METH_VARARGS, "" },
    { "timer_create", (PyCFunction) obj_timer_create, METH_VARARGS, "" },
    { "remove_style_all", (PyCFunction) obj_remove_style_all, METH_NOARGS, "" },
    { "add_style", (PyCFunction) obj_add_style, METH_VARARGS, "" },
    { "clear_flag", (PyCFunction) obj_clear_flag, METH_VARARGS, "" },
    { "add_flag", (PyCFunction) obj_add_flag, METH_VARARGS, "" },
    { "set_pos", (PyCFunction) obj_set_pos, METH_VARARGS, "" },
    { "set_size", (PyCFunction) obj_set_size, METH_VARARGS, "" },
    { "set_style_line_width", (PyCFunction) obj_set_style_line_width, METH_VARARGS, "" },
    { "set_style_line_color", (PyCFunction) obj_set_style_line_color, METH_VARARGS, "" },
    { "set_style_opa", (PyCFunction) obj_set_style_opa, METH_VARARGS, "" },
    { "move_foreground", (PyCFunction) obj_move_foreground, METH_NOARGS, "" },
    { NULL }
};

PyTypeObject obj_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lv.obj",
    .tp_doc = PyDoc_STR("LVGL object"),
    .tp_basicsize = sizeof(obj_object_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = obj_new,
    .tp_init = (initproc) obj_init,
    .tp_dealloc = (destructor) obj_dealloc,
    .tp_members = NULL,
    .tp_methods = obj_methods,
};

lv_obj_t * python_lv_get_obj(PyObject *obj) {
    if (PyObject_TypeCheck(obj, &obj_type)) {
        obj_object_t *self = (obj_object_t *) obj;

        return self->obj;
    }

    return NULL;
}
