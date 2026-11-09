/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include "msg.h"
#include "util.h"
#include "events.h"
#include "queue.h"

static char         buf[512];

static void msg_set_text_v(uint32_t event_code, const char *fmt, va_list args) {
    vsnprintf(buf, sizeof(buf), fmt, args);
    queue_send(NULL, event_code, buf);
}

void msg_set_text_fmt(const char * fmt, ...) {
    va_list args;

    va_start(args, fmt);
    msg_set_text_v(EVENT_MSG_UPDATE, fmt, args);
    va_end(args);
}

void msg_set_text_long_fmt(const char * fmt, ...) {
    va_list args;

    va_start(args, fmt);
    msg_set_text_v(EVENT_MSG_LONG_UPDATE, fmt, args);
    va_end(args);
}
