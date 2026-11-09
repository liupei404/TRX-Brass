/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <pthread.h>
#include <stdlib.h>
#include "backlight.h"
#include "queue.h"
#include "events.h"
#include "msgs.h"

typedef struct item_t {
    lv_obj_t        *obj;
    lv_event_code_t event_code;
    void            *param;
    struct item_t   *next;
} item_t;

static struct item_t    *head = NULL;
static struct item_t    *tail = NULL;
static pthread_mutex_t  mux;

void queue_init() {
    pthread_mutex_init(&mux, NULL);
}

void queue_work() {
    pthread_mutex_lock(&mux);

    while (head != NULL) {
        item_t *item = head;

        head = item->next;
        if (head == NULL) {
            tail = NULL;
        }

        pthread_mutex_unlock(&mux);

        if (item->event_code == LV_EVENT_REFRESH) {
            if (backlight_is_on() && lv_obj_is_valid(item->obj)) {
                lv_obj_invalidate(item->obj);
            }
        } else if (item->event_code == EVENT_MSG_UPDATE) {
            brass_msg_send(MSG_MSG, item->param);
            item->param = NULL;
        } else if (item->event_code == EVENT_MSG_LONG_UPDATE) {
            brass_msg_send(MSG_MSG_LONG, item->param);
            item->param = NULL;
        } else if (item->event_code == EVENT_MSG_TINY_UPDATE) {
            brass_msg_send(MSG_MSG_TINY, item->param);
            item->param = NULL;
        } else if (item->obj != NULL && lv_obj_is_valid(item->obj)) {
            lv_obj_send_event(item->obj, item->event_code, item->param);
        }

        pthread_mutex_lock(&mux);

        if (item->param != NULL) {
            free(item->param);
        }

        free(item);
    }

    pthread_mutex_unlock(&mux);
}

void queue_send(lv_obj_t *obj, lv_event_code_t event_code, void *param) {
    item_t *item = malloc(sizeof(item_t));

    if (item == NULL) {
        return;
    }

    item->obj = obj;
    item->event_code = event_code;
    item->param = param;
    item->next = NULL;

    pthread_mutex_lock(&mux);

    if (head == NULL && tail == NULL) {
        head = tail = item;
    } else {
        tail->next = item;
        tail = item;
    }

    pthread_mutex_unlock(&mux);
}

void queue_cancel(lv_obj_t *obj, lv_event_code_t event_code, void (*free_param)(void *)) {
    pthread_mutex_lock(&mux);

    item_t **link = &head;
    while (*link != NULL) {
        item_t *item = *link;

        if (item->obj == obj && item->event_code == event_code) {
            *link = item->next;
            if (tail == item) tail = NULL;
            if (item->param != NULL) {
                if (free_param != NULL) free_param(item->param);
                else free(item->param);
            }
            free(item);
        } else {
            link = &item->next;
        }
    }

    if (head != NULL && tail == NULL) {
        tail = head;
        while (tail->next != NULL) tail = tail->next;
    }

    pthread_mutex_unlock(&mux);
}
