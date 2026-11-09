/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include "python_lv.h"
#include "python_lv_object.h"
#include "python_trx.h"
#include "src/msgs.h"
#include "src/events.h"
#include "src/widgets/lv_spectrum.h"
#include "src/widgets/lv_spectrum3d.h"
#include "src/widgets/lv_waterfall.h"
#include "src/widgets/lv_finder.h"
#include "src/widgets/lv_xmeter.h"
#include "src/main.h"
#include "src/msgs.h"
#include "src/radio.h"
#include "src/buttons.h"
#include "src/dsp/agc.h"

/* Spectrum */

static void spectrum_msg_cb(void *s, brass_msg_t *m) {
    lv_obj_t *spectrum = brass_msg_get_user_data(m);

    switch (brass_msg_get_id(m)) {
        case MSG_FREQ_FFT_SHIFT: {
            const int32_t *df = brass_msg_get_payload(m);

            lv_spectrum_scroll_data(spectrum, *df);
        } break;

        case MSG_RATE_FFT_CHANGED: {
            const uint8_t *zoom = brass_msg_get_payload(m);

            lv_spectrum_set_span(spectrum, 100000 / *zoom);
            lv_spectrum_clear_data(spectrum);
        } break;

        case MSG_SPECTRUM_AUTO: {
            const msgs_auto_t *msg = brass_msg_get_payload(m);

            lv_spectrum_set_min(spectrum, msg->min + 3.0f);
            lv_spectrum_set_max(spectrum, msg->max + 10.0f);
        } break;

        case MSG_SPECTRUM_DATA: {
            const msgs_floats_t *msg = brass_msg_get_payload(m);

            lv_spectrum_add_data(spectrum, msg->data, msg->size);
            lv_obj_invalidate(spectrum);
        } break;
    }
}

static PyObject * trx_connect_spectrum(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *spectrum = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_FREQ_FFT_SHIFT, spectrum, spectrum_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_RATE_FFT_CHANGED, spectrum, spectrum_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_SPECTRUM_AUTO, spectrum, spectrum_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_SPECTRUM_DATA, spectrum, spectrum_msg_cb);
    }

    Py_RETURN_NONE;
}

/* Spectrum3D */

static void spectrum3d_msg_cb(void *s, brass_msg_t *m) {
    lv_obj_t *spectrum3d = brass_msg_get_user_data(m);

    switch (brass_msg_get_id(m)) {
        case MSG_FREQ_FFT_SHIFT: {
            const int32_t *df = brass_msg_get_payload(m);

            lv_spectrum3d_scroll_data(spectrum3d, *df);
        } break;

        case MSG_RATE_FFT_CHANGED: {
            const uint8_t *zoom = brass_msg_get_payload(m);

            lv_spectrum3d_set_span(spectrum3d, 100000 / *zoom);
            lv_spectrum3d_clear_data(spectrum3d);
        } break;

        case MSG_WATERFALL_AUTO: {
            const msgs_auto_t *msg = brass_msg_get_payload(m);

            lv_spectrum3d_set_min(spectrum3d, msg->min + 3.0f);
            lv_spectrum3d_set_max(spectrum3d, msg->max + 3.0f);
        } break;

        case MSG_WATERFALL_DATA: {
            const msgs_floats_t *msg = brass_msg_get_payload(m);

            lv_spectrum3d_add_data(spectrum3d, msg->data, msg->size);
            lv_obj_invalidate(spectrum3d);
        } break;
    }
}

static PyObject * trx_connect_spectrum3d(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *spectrum3d = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_FREQ_FFT_SHIFT, spectrum3d, spectrum3d_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_RATE_FFT_CHANGED, spectrum3d, spectrum3d_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_WATERFALL_AUTO, spectrum3d, spectrum3d_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_WATERFALL_DATA, spectrum3d, spectrum3d_msg_cb);
    }

    Py_RETURN_NONE;
}

/* Waterfall */

static void waterfall_msg_cb(void *s, brass_msg_t *m) {
    lv_obj_t *waterfall = brass_msg_get_user_data(m);

    switch (brass_msg_get_id(m)) {
        case MSG_BAND_CHANGED:
            lv_waterfall_clear_data(waterfall);
            break;

        case MSG_FREQ_FFT_SHIFT: {
            const int32_t   *df = brass_msg_get_payload(m);

            lv_waterfall_scroll_data(waterfall, *df);
        } break;

        case MSG_RATE_FFT_CHANGED: {
            const uint8_t *zoom = brass_msg_get_payload(m);

            lv_waterfall_set_span(waterfall, 100000 / *zoom);
            lv_waterfall_clear_data(waterfall);
        } break;

        case MSG_WATERFALL_AUTO: {
            const msgs_auto_t *msg = brass_msg_get_payload(m);

            lv_waterfall_set_min(waterfall, msg->min + 3.0f);
            lv_waterfall_set_max(waterfall, msg->max + 3.0f);
        } break;

        case MSG_WATERFALL_DATA: {
            const msgs_floats_t *msg = brass_msg_get_payload(m);

            lv_waterfall_add_data(waterfall, msg->data, msg->size);
            lv_obj_invalidate(waterfall);
        } break;
    }
}

static PyObject * trx_connect_waterfall(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *waterfall = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_FREQ_FFT_SHIFT, waterfall, waterfall_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_RATE_FFT_CHANGED, waterfall, waterfall_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_WATERFALL_AUTO, waterfall, waterfall_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_BAND_CHANGED, waterfall, waterfall_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_WATERFALL_DATA, waterfall, waterfall_msg_cb);
    }

    Py_RETURN_NONE;
}

/* Finder */

static void finder_event_cb(void *s, brass_msg_t *m) {
    lv_obj_t *finder = brass_msg_get_user_data(m);

    switch (brass_msg_get_id(m)) {
        case MSG_FILTER_CHANGED: {
            int32_t from, to;

            radio_filter_get(&from, &to);
            lv_finder_set_offsets(finder, from, to);
            lv_obj_invalidate(finder);
        } break;

        case MSG_RATE_FFT_CHANGED: {
            const uint8_t *zoom = brass_msg_get_payload(m);

            lv_finder_set_span(finder, 100000 / *zoom);
            lv_obj_invalidate(finder);
        } break;

        case MSG_FREQ_FFT_CHANGED: {
            const uint64_t  *freq = brass_msg_get_payload(m);

            lv_finder_set_center(finder, *freq);
            lv_obj_invalidate(finder);
        } break;

        case MSG_FINDER_CURSOR: {
            const msg_finder_cursor_t *msg = brass_msg_get_payload(m);

            lv_finder_set_cursor_num(finder, msg->num);
            lv_finder_set_cursor(finder, 1, msg->cursor[0]);
            lv_finder_set_cursor(finder, 2, msg->cursor[1]);
            lv_finder_set_cursor(finder, 3, msg->cursor[2]);
        } break;

        default:
            break;
    }
}

static void rx_finder_event_cb(void *s, brass_msg_t *m) {
    lv_obj_t *finder = brass_msg_get_user_data(m);

    switch (brass_msg_get_id(m)) {
        case MSG_FREQ_RX_CHANGED: {
            const uint64_t *freq = brass_msg_get_payload(m);

            lv_finder_set_value(finder, *freq);
        } break;

        default:
            break;
    }
}

static void tx_finder_event_cb(void *s, brass_msg_t *m) {
    lv_obj_t *finder = brass_msg_get_user_data(m);

    switch (brass_msg_get_id(m)) {
        case MSG_FREQ_TX_CHANGED: {
            const uint64_t *freq = brass_msg_get_payload(m);

            lv_finder_set_value(finder, *freq);
        } break;

        default:
            break;
    }
}

static PyObject * trx_connect_rx_finder(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *finder = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_FILTER_CHANGED, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_RATE_FFT_CHANGED, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_FREQ_FFT_CHANGED, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_FINDER_CURSOR, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_FREQ_RX_CHANGED, finder, rx_finder_event_cb);
    }

    Py_RETURN_NONE;
}

static PyObject * trx_connect_tx_finder(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *finder = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_FILTER_CHANGED, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_RATE_FFT_CHANGED, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_FREQ_FFT_CHANGED, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_FINDER_CURSOR, finder, finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_FREQ_TX_CHANGED, finder, tx_finder_event_cb);
        brass_msg_subscribe_obj_cb(MSG_FINDER_CURSOR, finder, tx_finder_event_cb);
    }

    Py_RETURN_NONE;
}

/* X-Meter */

static void xmeter_msg_cb(void *s, brass_msg_t *m) {
    lv_obj_t *xmeter = brass_msg_get_user_data(m);

    switch (brass_msg_get_id(m)) {
        case MSG_SPECTRUM_AUTO: {
            const msgs_auto_t *msg = brass_msg_get_payload(m);

            lv_xmeter_set_part(xmeter, 0, msg->min);
        } break;

        case MSG_MIC_METER:
        case MSG_SWR_METER:
        case MSG_PWR_METER:
        case MSG_SMETER: {
            const float *msg = brass_msg_get_payload(m);

            lv_xmeter_set_value(xmeter, *msg);
        }
    }
}

static PyObject * trx_connect_smeter(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *xmeter = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_SPECTRUM_AUTO, xmeter, xmeter_msg_cb);
        brass_msg_subscribe_obj_cb(MSG_SMETER, xmeter, xmeter_msg_cb);
    }

    Py_RETURN_NONE;
}

static PyObject * trx_connect_mic_meter(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *xmeter = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_MIC_METER, xmeter, xmeter_msg_cb);
    }

    Py_RETURN_NONE;
}

static PyObject * trx_connect_swr_meter(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *xmeter = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_SWR_METER, xmeter, xmeter_msg_cb);
    }

    Py_RETURN_NONE;
}

static PyObject * trx_connect_pwr_meter(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "O", &obj)) {
        lv_obj_t *xmeter = python_lv_get_obj(obj);

        brass_msg_subscribe_obj_cb(MSG_PWR_METER, xmeter, xmeter_msg_cb);
    }

    Py_RETURN_NONE;
}

/* * */

static PyObject * trx_connect_button(PyObject *self, PyObject *args) {
    LV_LOG_INFO("begin");

    int         index = -1;
    PyObject    *obj = NULL;

    if (PyArg_ParseTuple(args, "iO", &index, &obj)) {
        lv_obj_t *button = python_lv_get_obj(obj);

        buttons_insert(index, button);
    }

    Py_RETURN_NONE;
}

/* * */

static PyMethodDef trx_methods[] = {
    { "connect_spectrum", (PyCFunction) trx_connect_spectrum, METH_VARARGS, "" },
    { "connect_spectrum3d", (PyCFunction) trx_connect_spectrum3d, METH_VARARGS, "" },
    { "connect_waterfall", (PyCFunction) trx_connect_waterfall, METH_VARARGS, "" },
    { "connect_rx_finder", (PyCFunction) trx_connect_rx_finder, METH_VARARGS, "" },
    { "connect_tx_finder", (PyCFunction) trx_connect_tx_finder, METH_VARARGS, "" },
    { "connect_button", (PyCFunction) trx_connect_button, METH_VARARGS, "" },
    { "connect_smeter", (PyCFunction) trx_connect_smeter, METH_VARARGS, "" },
    { "connect_mic_meter", (PyCFunction) trx_connect_mic_meter, METH_VARARGS, "" },
    { "connect_swr_meter", (PyCFunction) trx_connect_swr_meter, METH_VARARGS, "" },
    { "connect_pwr_meter", (PyCFunction) trx_connect_pwr_meter, METH_VARARGS, "" },
    { NULL }
};

static PyModuleDef trx_module = {
    PyModuleDef_HEAD_INIT,
    "trx",
    NULL,
    -1,
    trx_methods
};

PyMODINIT_FUNC PyInit_trx() {
    PyObject *m = PyModule_Create(&trx_module);

    /* Messages */

    PyModule_AddObjectRef(m, "MSG_MODE_CHANGED",        PyLong_FromLong(MSG_MODE_CHANGED));
    PyModule_AddObjectRef(m, "MSG_FILTER_CHANGED",      PyLong_FromLong(MSG_FILTER_CHANGED));
    PyModule_AddObjectRef(m, "MSG_BAND_CHANGED",        PyLong_FromLong(MSG_BAND_CHANGED));
    PyModule_AddObjectRef(m, "MSG_FREQ_RX_PRE_CHANGED", PyLong_FromLong(MSG_FREQ_RX_PRE_CHANGED));
    PyModule_AddObjectRef(m, "MSG_FREQ_RX_CHANGED",     PyLong_FromLong(MSG_FREQ_RX_CHANGED));
    PyModule_AddObjectRef(m, "MSG_FREQ_TX_PRE_CHANGED", PyLong_FromLong(MSG_FREQ_TX_PRE_CHANGED));
    PyModule_AddObjectRef(m, "MSG_FREQ_TX_CHANGED",     PyLong_FromLong(MSG_FREQ_TX_CHANGED));
    PyModule_AddObjectRef(m, "MSG_FREQ_FFT_CHANGED",    PyLong_FromLong(MSG_FREQ_FFT_CHANGED));
    PyModule_AddObjectRef(m, "MSG_FREQ_FFT_SHIFT",      PyLong_FromLong(MSG_FREQ_FFT_SHIFT));
    PyModule_AddObjectRef(m, "MSG_RATE_FFT_CHANGED",    PyLong_FromLong(MSG_RATE_FFT_CHANGED));
    PyModule_AddObjectRef(m, "MSG_SPECTRUM_AUTO",       PyLong_FromLong(MSG_SPECTRUM_AUTO));
    PyModule_AddObjectRef(m, "MSG_WATERFALL_AUTO",      PyLong_FromLong(MSG_WATERFALL_AUTO));
    PyModule_AddObjectRef(m, "MSG_SPECTRUM_DATA",       PyLong_FromLong(MSG_SPECTRUM_DATA));
    PyModule_AddObjectRef(m, "MSG_WATERFALL_DATA",      PyLong_FromLong(MSG_WATERFALL_DATA));
    PyModule_AddObjectRef(m, "MSG_PTT",                 PyLong_FromLong(MSG_PTT));
    PyModule_AddObjectRef(m, "MSG_TX",                  PyLong_FromLong(MSG_TX));
    PyModule_AddObjectRef(m, "MSG_RX",                  PyLong_FromLong(MSG_RX));
    PyModule_AddObjectRef(m, "MSG_FINDER_CURSOR",       PyLong_FromLong(MSG_FINDER_CURSOR));
    PyModule_AddObjectRef(m, "MSG_RECORDER",            PyLong_FromLong(MSG_RECORDER));
    PyModule_AddObjectRef(m, "MSG_MSG",                 PyLong_FromLong(MSG_MSG));
    PyModule_AddObjectRef(m, "MSG_MSG_LONG",            PyLong_FromLong(MSG_MSG_LONG));
    PyModule_AddObjectRef(m, "MSG_MSG_TINY",            PyLong_FromLong(MSG_MSG_TINY));
    PyModule_AddObjectRef(m, "MSG_SMETER",              PyLong_FromLong(MSG_SMETER));
    PyModule_AddObjectRef(m, "MSG_AGC_CHANGED",         PyLong_FromLong(MSG_AGC_CHANGED));
    PyModule_AddObjectRef(m, "MSG_ANT_CHANGED",         PyLong_FromLong(MSG_ANT_CHANGED));
    PyModule_AddObjectRef(m, "MSG_SPLIT_CHANGED",       PyLong_FromLong(MSG_SPLIT_CHANGED));
    PyModule_AddObjectRef(m, "MSG_MIC_METER",           PyLong_FromLong(MSG_MIC_METER));
    PyModule_AddObjectRef(m, "MSG_MIC",                 PyLong_FromLong(MSG_MIC));
    PyModule_AddObjectRef(m, "MSG_SWR_METER",           PyLong_FromLong(MSG_SWR_METER));
    PyModule_AddObjectRef(m, "MSG_PWR_METER",           PyLong_FromLong(MSG_PWR_METER));

    /* Modes */

    PyModule_AddObjectRef(m, "RADIO_MODE_LSB",          PyLong_FromLong(RADIO_MODE_LSB));
    PyModule_AddObjectRef(m, "RADIO_MODE_USB",          PyLong_FromLong(RADIO_MODE_USB));
    PyModule_AddObjectRef(m, "RADIO_MODE_CW",           PyLong_FromLong(RADIO_MODE_CW));
    PyModule_AddObjectRef(m, "RADIO_MODE_CWR",          PyLong_FromLong(RADIO_MODE_CWR));
    PyModule_AddObjectRef(m, "RADIO_MODE_AM",           PyLong_FromLong(RADIO_MODE_AM));
    PyModule_AddObjectRef(m, "RADIO_MODE_NFM",          PyLong_FromLong(RADIO_MODE_NFM));
    PyModule_AddObjectRef(m, "RADIO_MODE_RTTY",         PyLong_FromLong(RADIO_MODE_RTTY));
    PyModule_AddObjectRef(m, "RADIO_MODE_OLIVIA",       PyLong_FromLong(RADIO_MODE_OLIVIA));
    PyModule_AddObjectRef(m, "RADIO_MODE_DMR",          PyLong_FromLong(RADIO_MODE_DMR));

    /* Split */

    PyModule_AddObjectRef(m, "SPLIT_NONE",              PyLong_FromLong(SPLIT_NONE));
    PyModule_AddObjectRef(m, "SPLIT_RX",                PyLong_FromLong(SPLIT_RX));
    PyModule_AddObjectRef(m, "SPLIT_TX",                PyLong_FromLong(SPLIT_TX));

    /* AGC */

    PyModule_AddObjectRef(m, "AGC_OFF",                 PyLong_FromLong(AGC_OFF));
    PyModule_AddObjectRef(m, "AGC_LONG",                PyLong_FromLong(AGC_LONG));
    PyModule_AddObjectRef(m, "AGC_SLOW",                PyLong_FromLong(AGC_SLOW));
    PyModule_AddObjectRef(m, "AGC_MED",                 PyLong_FromLong(AGC_MED));
    PyModule_AddObjectRef(m, "AGC_FAST",                PyLong_FromLong(AGC_FAST));
    PyModule_AddObjectRef(m, "AGC_CUSTOM",              PyLong_FromLong(AGC_CUSTOM));

    return m;
}
