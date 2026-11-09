/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include "msg_tiny.h"
#include "util.h"
#include "events.h"
#include "queue.h"

static char         buf[512];

void msg_tiny_set_text_fmt(const char * fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    queue_send(NULL, EVENT_MSG_TINY_UPDATE, buf);
}
