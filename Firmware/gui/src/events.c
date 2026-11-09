/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include "events.h"
#include "backlight.h"
#include "keyboard.h"
#include "dialog.h"
#include "main.h"

uint32_t        EVENT_ROTARY;
uint32_t        EVENT_KEYPAD;
uint32_t        EVENT_HKEY;
uint32_t        EVENT_RADIO_TX;
uint32_t        EVENT_RADIO_RX;
uint32_t        EVENT_PANNEL_UPDATE;
uint32_t        EVENT_MSG_UPDATE;
uint32_t        EVENT_MSG_LONG_UPDATE;
uint32_t        EVENT_MSG_TINY_UPDATE;
uint32_t        EVENT_FREQ_UPDATE;
uint32_t        EVENT_FT8_MSG;
uint32_t        EVENT_SSTV_VIS;
uint32_t        EVENT_SSTV_LINE;
uint32_t        EVENT_SSTV_FRAME;
uint32_t        EVENT_GPS;
uint32_t        EVENT_VOICE_DONE;
uint32_t        EVENT_BLUETOOTH_MSG;

void event_init() {
    EVENT_ROTARY = lv_event_register_id();
    EVENT_KEYPAD = lv_event_register_id();
    EVENT_HKEY = lv_event_register_id();
    EVENT_RADIO_TX = lv_event_register_id();
    EVENT_RADIO_RX = lv_event_register_id();
    EVENT_PANNEL_UPDATE = lv_event_register_id();
    EVENT_MSG_UPDATE = lv_event_register_id();
    EVENT_MSG_LONG_UPDATE = lv_event_register_id();
    EVENT_MSG_TINY_UPDATE = lv_event_register_id();
    EVENT_FREQ_UPDATE = lv_event_register_id();
    EVENT_FT8_MSG = lv_event_register_id();
    EVENT_SSTV_VIS = lv_event_register_id();
    EVENT_SSTV_LINE = lv_event_register_id();
    EVENT_SSTV_FRAME = lv_event_register_id();
    EVENT_GPS = lv_event_register_id();
    EVENT_VOICE_DONE = lv_event_register_id();
    EVENT_BLUETOOTH_MSG = lv_event_register_id();
}
