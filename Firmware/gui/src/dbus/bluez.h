/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdbool.h>
#include <gio/gio.h>

void dbus_bluez_check(GDBusConnection *conn);
void dbus_bluez_init();
bool dbus_bluez_ok();
bool dbus_bluez_scan(bool on);
void dbus_bluez_connect(const char *path);
bool dbus_bluez_remove(const char *path);
void dbus_bluez_cached();
bool dbus_bluez_register_agent();

void dbus_bluez_interface_added(
    GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data);
