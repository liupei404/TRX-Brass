/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Application message bus used independently of LVGL.
 */

#pragma once

#include <stdint.h>
#include "lvgl/lvgl.h"

typedef struct brass_msg {
    uint32_t id;
    const void *payload;
    void *user_data;
} brass_msg_t;

typedef struct brass_msg_subscription brass_msg_subscription_t;
typedef void (*brass_msg_cb_t)(void *subscriber, brass_msg_t *msg);

#define BRASS_EVENT_MSG_RECEIVED ((lv_event_code_t) (LV_EVENT_LAST + 1))

/* Synchronous dispatch: payload is borrowed for the duration of this call.
 * The bus is not thread-safe. Dispatch to UI subscribers from the LVGL thread
 * (e.g. a timer callback), or while holding brass_lv_lock() outside rendering.
 */
void brass_msg_send(uint32_t id, const void *payload);
brass_msg_subscription_t *brass_msg_subscribe(uint32_t id, brass_msg_cb_t callback, void *user_data);
brass_msg_subscription_t *brass_msg_subscribe_obj(uint32_t id, lv_obj_t *obj, void *user_data);
brass_msg_subscription_t *brass_msg_subscribe_obj_cb(uint32_t id, lv_obj_t *obj, brass_msg_cb_t callback);
void brass_msg_unsubscribe(brass_msg_subscription_t *subscription);

static inline uint32_t brass_msg_get_id(const brass_msg_t *msg) {
    return msg->id;
}

static inline const void *brass_msg_get_payload(const brass_msg_t *msg) {
    return msg->payload;
}

static inline void *brass_msg_get_user_data(const brass_msg_t *msg) {
    return msg->user_data;
}

static inline brass_msg_t *brass_event_get_msg(lv_event_t *event) {
    return lv_event_get_param(event);
}
