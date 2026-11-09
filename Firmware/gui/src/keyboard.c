/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lv_drivers/indev/xkb.h"

#include "device_monitor.h"
#include "keyboard.h"
#include "msg.h"

#define BITS_PER_LONG   (sizeof(unsigned long) * 8U)
#define BITS_TO_LONGS(n) (((n) + BITS_PER_LONG - 1U) / BITS_PER_LONG)

typedef struct keyboard_device_t {
    char                        *syspath;
    char                        *devnode;
    lv_indev_t                  *indev;
    xkb_drv_state_t             xkb;
    int                         fd;
    uint32_t                    key;
    lv_indev_state_t            state;
    bool                        usb;
    bool                        bluetooth;
    bool                        deleting;
    struct keyboard_device_t    *next;
} keyboard_device_t;

lv_group_t *keyboard_group;

static keyboard_device_t *devices = NULL;

static void delete_disconnected_device(void *user_data) {
    keyboard_device_t *item = user_data;

    item->deleting = false;
    if (item->indev != NULL) lv_indev_delete(item->indev);
}

static void unlink_device(keyboard_device_t *item) {
    keyboard_device_t **link = &devices;

    while (*link != NULL) {
        if (*link == item) {
            *link = item->next;
            free(item->syspath);
            free(item->devnode);
            free(item);
            return;
        }

        link = &(*link)->next;
    }
}

static void indev_delete_cb(lv_event_t *event) {
    keyboard_device_t *item = lv_event_get_user_data(event);

    if (item->deleting) {
        lv_async_call_cancel(delete_disconnected_device, item);
        item->deleting = false;
    }
    if (item->usb) msg_set_text_fmt("USB keyboard disconnected");
    if (item->bluetooth) msg_set_text_fmt("BT keyboard disconnected");
    if (item->fd >= 0) close(item->fd);
    xkb_deinit_state(&item->xkb);
    item->indev = NULL;
    unlink_device(item);
}

static void keyboard_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    keyboard_device_t *item = lv_indev_get_user_data(indev);
    struct input_event event;
    ssize_t size;

    while ((size = read(item->fd, &event, sizeof(event))) == sizeof(event)) {
        if (event.type != EV_KEY) continue;

        uint32_t key = xkb_process_key_state(&item->xkb, event.code, event.value);
        if (key == 0) continue;

        item->key = key;
        item->state = event.value == 0 ? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
        data->continue_reading = true;
        break;
    }

    if (size < 0 && (errno == ENODEV || errno == EIO) && !item->deleting) {
        item->deleting = true;
        lv_async_call(delete_disconnected_device, item);
    }

    data->key = item->key;
    data->state = item->state;
}

static bool bit_is_set(const unsigned long *bits, unsigned int bit) {
    return (bits[bit / BITS_PER_LONG] & (1UL << (bit % BITS_PER_LONG))) != 0;
}

static bool is_full_keyboard(const char *devnode, char *name, size_t name_size) {
    unsigned long event_bits[BITS_TO_LONGS(EV_MAX + 1)] = {0};
    unsigned long key_bits[BITS_TO_LONGS(KEY_MAX + 1)] = {0};

    int fd = open(devnode, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return false;

    bool result = false;
    if (ioctl(fd, EVIOCGBIT(0, sizeof(event_bits)), event_bits) >= 0 &&
        bit_is_set(event_bits, EV_KEY) &&
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) >= 0 &&
        bit_is_set(key_bits, KEY_Q) &&
        bit_is_set(key_bits, KEY_A) &&
        bit_is_set(key_bits, KEY_Z) &&
        bit_is_set(key_bits, KEY_SPACE) &&
        bit_is_set(key_bits, KEY_ENTER)) {
        result = true;
        if (name != NULL && name_size > 0) {
            if (ioctl(fd, EVIOCGNAME(name_size), name) < 0) name[0] = '\0';
        }
    }

    close(fd);
    return result;
}

static keyboard_device_t *find_device(const char *syspath) {
    for (keyboard_device_t *item = devices; item != NULL; item = item->next) {
        if (strcmp(item->syspath, syspath) == 0) return item;
    }

    return NULL;
}

static void add_device(struct udev_device *device) {
    const char *syspath = udev_device_get_syspath(device);
    const char *devnode = udev_device_get_devnode(device);
    const char *sysname = udev_device_get_sysname(device);

    if (syspath == NULL || devnode == NULL || sysname == NULL ||
        strncmp(sysname, "event", 5) != 0 || find_device(syspath) != NULL) {
        return;
    }

    char name[128] = "";
    if (!is_full_keyboard(devnode, name, sizeof(name))) return;

    int fd = open(devnode, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        LV_LOG_ERROR("keyboard: unable to open %s", devnode);
        return;
    }

    keyboard_device_t *item = calloc(1, sizeof(*item));
    if (item == NULL) {
        close(fd);
        return;
    }

    item->fd = fd;
    item->state = LV_INDEV_STATE_RELEASED;

    if (!xkb_init_state(&item->xkb)) {
        LV_LOG_ERROR("keyboard: unable to initialize XKB for %s", devnode);
        close(fd);
        free(item);
        return;
    }

    item->syspath = strdup(syspath);
    item->devnode = strdup(devnode);
    if (item->syspath == NULL || item->devnode == NULL) {
        free(item->syspath);
        free(item->devnode);
        xkb_deinit_state(&item->xkb);
        close(fd);
        free(item);
        return;
    }

    lv_indev_t *indev = lv_indev_create();
    if (indev == NULL) {
        free(item->syspath);
        free(item->devnode);
        xkb_deinit_state(&item->xkb);
        close(fd);
        free(item);
        return;
    }

    item->indev = indev;
    const char *bus = udev_device_get_property_value(device, "ID_BUS");
    item->bluetooth = bus != NULL && strcmp(bus, "bluetooth") == 0;
    item->usb = !item->bluetooth &&
        udev_device_get_parent_with_subsystem_devtype(device, "usb", "usb_device") != NULL;
    item->next = devices;
    devices = item;

    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, keyboard_read_cb);
    lv_indev_set_user_data(indev, item);
    lv_indev_set_group(indev, keyboard_group);
    lv_indev_add_event_cb(indev, indev_delete_cb, LV_EVENT_DELETE, item);
    LV_LOG_INFO("keyboard added: %s (%s)", name[0] != '\0' ? name : "unknown", devnode);
    if (item->usb) msg_set_text_fmt("USB keyboard connected");
    if (item->bluetooth) msg_set_text_fmt("BT keyboard connected");
}

static void remove_device(const char *syspath) {
    keyboard_device_t **link = &devices;

    while (*link != NULL) {
        keyboard_device_t *item = *link;

        if (strcmp(item->syspath, syspath) == 0) {
            LV_LOG_INFO("keyboard removed: %s", item->devnode);
            lv_indev_delete(item->indev);
            return;
        }

        link = &item->next;
    }
}

static void input_device_cb(struct udev_device *device, const char *action, void *user_data) {
    (void)user_data;

    const char *syspath = udev_device_get_syspath(device);
    if (syspath == NULL) return;

    if (strcmp(action, "remove") == 0) {
        remove_device(syspath);
    } else if (strcmp(action, "add") == 0 || strcmp(action, "change") == 0) {
        add_device(device);
    }
}

void keyboard_init() {
    keyboard_group = lv_group_create();

    if (!device_monitor_subscribe("input", input_device_cb, NULL)) {
        LV_LOG_ERROR("keyboard: unable to subscribe to input hotplug");
        return;
    }

    device_monitor_enumerate("input", input_device_cb, NULL);
}

bool keyboard_ready() {
    return devices != NULL;
}
