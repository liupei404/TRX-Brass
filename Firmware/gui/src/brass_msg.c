/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <stdbool.h>
#include <stdlib.h>

#include "brass_msg.h"

struct brass_msg_subscription {
    struct brass_msg_subscription *next;
    uint32_t id;
    brass_msg_cb_t callback;
    void *user_data;
    bool deleted;
};

static brass_msg_subscription_t *subscriptions;
static unsigned dispatch_depth;

static void cleanup_subscriptions(void) {
    brass_msg_subscription_t **link = &subscriptions;

    while (*link != NULL) {
        brass_msg_subscription_t *subscription = *link;

        if (subscription->deleted) {
            *link = subscription->next;
            free(subscription);
        } else {
            link = &subscription->next;
        }
    }
}

brass_msg_subscription_t *brass_msg_subscribe(uint32_t id, brass_msg_cb_t callback, void *user_data) {
    if (callback == NULL) return NULL;

    brass_msg_subscription_t *subscription = malloc(sizeof(*subscription));
    if (subscription == NULL) return NULL;

    subscription->id = id;
    subscription->callback = callback;
    subscription->user_data = user_data;
    subscription->deleted = false;
    subscription->next = subscriptions;
    subscriptions = subscription;

    return subscription;
}

void brass_msg_unsubscribe(brass_msg_subscription_t *subscription) {
    if (subscription == NULL || subscription->deleted) return;

    subscription->deleted = true;
    if (dispatch_depth == 0) cleanup_subscriptions();
}

void brass_msg_send(uint32_t id, const void *payload) {
    dispatch_depth++;

    for (brass_msg_subscription_t *subscription = subscriptions;
         subscription != NULL;
         subscription = subscription->next) {
        if (subscription->deleted || subscription->id != id) continue;

        brass_msg_t msg = {
            .id = id,
            .payload = payload,
            .user_data = subscription->user_data,
        };
        subscription->callback(subscription, &msg);
    }

    dispatch_depth--;
    if (dispatch_depth == 0) cleanup_subscriptions();
}

static void object_msg_cb(void *subscriber, brass_msg_t *msg) {
    (void) subscriber;
    lv_obj_send_event(msg->user_data, BRASS_EVENT_MSG_RECEIVED, msg);
}

static void object_delete_cb(lv_event_t *event) {
    brass_msg_unsubscribe(lv_event_get_user_data(event));
}

brass_msg_subscription_t *brass_msg_subscribe_obj(uint32_t id, lv_obj_t *obj, void *user_data) {
    (void) user_data;

    if (obj == NULL) return NULL;

    brass_msg_subscription_t *subscription = brass_msg_subscribe(id, object_msg_cb, obj);
    if (subscription != NULL) {
        lv_obj_add_event_cb(obj, object_delete_cb, LV_EVENT_DELETE, subscription);
    }

    return subscription;
}

brass_msg_subscription_t *brass_msg_subscribe_obj_cb(uint32_t id, lv_obj_t *obj, brass_msg_cb_t callback) {
    if (obj == NULL || callback == NULL) return NULL;

    brass_msg_subscription_t *subscription = brass_msg_subscribe(id, callback, obj);
    if (subscription != NULL) {
        lv_obj_add_event_cb(obj, object_delete_cb, LV_EVENT_DELETE, subscription);
    }

    return subscription;
}
