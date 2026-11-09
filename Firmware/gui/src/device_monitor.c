/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "device_monitor.h"

typedef struct device_subscription_t {
    char                            *subsystem;
    device_monitor_cb_t             callback;
    void                            *user_data;
    struct device_subscription_t    *next;
} device_subscription_t;

static struct udev             *udev_ctx = NULL;
static struct udev_monitor     *monitor = NULL;
static device_subscription_t   *subscriptions = NULL;
static lv_timer_t              *timer = NULL;

static void dispatch(struct udev_device *device, const char *action) {
    const char *subsystem = udev_device_get_subsystem(device);

    for (device_subscription_t *item = subscriptions; item != NULL; item = item->next) {
        if (item->subsystem == NULL ||
            (subsystem != NULL && strcmp(item->subsystem, subsystem) == 0)) {
            item->callback(device, action, item->user_data);
        }
    }
}

static void monitor_cb(lv_timer_t *t) {
    (void)t;

    while (true) {
        struct udev_device *device = udev_monitor_receive_device(monitor);
        if (device == NULL) break;

        const char *action = udev_device_get_action(device);
        dispatch(device, action != NULL ? action : "change");
        udev_device_unref(device);
    }
}

bool device_monitor_init(void) {
    if (monitor != NULL) return true;

    udev_ctx = udev_new();
    if (udev_ctx == NULL) {
        LV_LOG_ERROR("udev: unable to create context");
        return false;
    }

    /* The udev channel delivers events after devnode creation and rule
     * processing. No external scripts or per-device rules are required. */
    monitor = udev_monitor_new_from_netlink(udev_ctx, "udev");
    if (monitor == NULL || udev_monitor_enable_receiving(monitor) < 0) {
        LV_LOG_ERROR("udev: unable to start monitor");
        if (monitor != NULL) udev_monitor_unref(monitor);
        monitor = NULL;
        udev_unref(udev_ctx);
        udev_ctx = NULL;
        return false;
    }

    int fd = udev_monitor_get_fd(monitor);
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    timer = lv_timer_create(monitor_cb, 100, NULL);
    if (timer == NULL) {
        udev_monitor_unref(monitor);
        monitor = NULL;
        udev_unref(udev_ctx);
        udev_ctx = NULL;
        return false;
    }

    return true;
}

bool device_monitor_subscribe(const char *subsystem, device_monitor_cb_t callback, void *user_data) {
    if (callback == NULL || !device_monitor_init()) return false;

    device_subscription_t *item = calloc(1, sizeof(*item));
    if (item == NULL) return false;

    if (subsystem != NULL) {
        item->subsystem = strdup(subsystem);
        if (item->subsystem == NULL) {
            free(item);
            return false;
        }
    }

    item->callback = callback;
    item->user_data = user_data;
    item->next = subscriptions;
    subscriptions = item;

    return true;
}

void device_monitor_enumerate(const char *subsystem, device_monitor_cb_t callback, void *user_data) {
    if (callback == NULL || !device_monitor_init()) return;

    struct udev_enumerate *enumerate = udev_enumerate_new(udev_ctx);
    if (enumerate == NULL) return;

    if (subsystem != NULL) udev_enumerate_add_match_subsystem(enumerate, subsystem);
    if (udev_enumerate_scan_devices(enumerate) < 0) {
        udev_enumerate_unref(enumerate);
        return;
    }

    struct udev_list_entry *entry;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(enumerate)) {
        const char *syspath = udev_list_entry_get_name(entry);
        struct udev_device *device = udev_device_new_from_syspath(udev_ctx, syspath);

        if (device != NULL) {
            callback(device, "add", user_data);
            udev_device_unref(device);
        }
    }

    udev_enumerate_unref(enumerate);
}
