/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <gio/gio.h>
#include "lvgl/lvgl.h"
#include "dialog.h"

extern dialog_t *dialog_bluetooth;

void dialog_bluetooth_interface_added(const gchar *addr, const gchar *name, const gchar *path);
