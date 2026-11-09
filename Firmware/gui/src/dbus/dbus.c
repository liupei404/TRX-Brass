/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <gio/gio.h>
#include <pthread.h>
#include "lvgl/lvgl.h"
#include "dbus.h"
#include "bluez.h"

static GDBusConnection  *connection = NULL;

void* loop_thread(void *arg) {
    GMainLoop *loop = (GMainLoop *)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    g_main_loop_run(loop);

    return NULL;
}

void dbus_init() {
    GError *error = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (connection) {
        dbus_bluez_check(connection);
        dbus_bluez_register_agent();

        GMainLoop   *loop = g_main_loop_new(NULL, FALSE);
        pthread_t   thread_id;

        g_dbus_connection_signal_subscribe(
            connection, "org.bluez", "org.freedesktop.DBus.ObjectManager",
            "InterfacesAdded", "/", NULL, G_DBUS_SIGNAL_FLAGS_NONE,
            dbus_bluez_interface_added, NULL, NULL
        );

        pthread_create(&thread_id, NULL, loop_thread, loop);
    } else {
        LV_LOG_ERROR("D-Bus error: %s", error->message);
        g_error_free(error);
    }
}
