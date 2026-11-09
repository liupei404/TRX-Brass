/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdbool.h>
#include <libudev.h>

typedef void (*device_monitor_cb_t)(struct udev_device *device, const char *action, void *user_data);

bool device_monitor_init(void);
bool device_monitor_subscribe(const char *subsystem, device_monitor_cb_t callback, void *user_data);
void device_monitor_enumerate(const char *subsystem, device_monitor_cb_t callback, void *user_data);
