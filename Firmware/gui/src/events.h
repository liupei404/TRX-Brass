/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include "lvgl/lvgl.h"

#include <unistd.h>
#include <stdint.h>

typedef enum {
    KEYPAD_UNKNOWN = 0,

    KEYPAD_POWER,
    KEYPAD_VOL,
    KEYPAD_MFK,
    KEYPAD_APP,

    KEYPAD_F1,
    KEYPAD_F2,
    KEYPAD_F3,
    KEYPAD_F4,
    KEYPAD_F5,
    KEYPAD_F6,

    KEYPAD_PTT,
    KEYPAD_BAND_DOWN,
    KEYPAD_BAND_UP,
    KEYPAD_MODE,
    KEYPAD_FST
} keypad_key_t;

typedef enum {
    KEYPAD_PRESS = 0,
    KEYPAD_RELEASE,
    KEYPAD_LONG,
    KEYPAD_LONG_RELEASE
} keypad_state_t;

typedef struct {
    keypad_key_t    key;
    keypad_state_t  state;
} event_keypad_t;

typedef enum {
    HKEY_NONE = 0,

    HKEY_UP,
    HKEY_DOWN,

    HKEY_0,
    HKEY_1,
    HKEY_2,
    HKEY_3,
    HKEY_4,
    HKEY_5,
    HKEY_6,
    HKEY_7,
    HKEY_8,
    HKEY_9,

    HKEY_STAR,
    HKEY_SHARP,

    HKEY_P1,
    HKEY_P2,
    HKEY_P3,
    HKEY_P4,

    HKEY_A,
    HKEY_B,
    HKEY_C,
    HKEY_D
} hkey_t;

typedef enum {
    HKEY_RELEASE = 0,
    HKEY_PRESS,
    HKEY_LONG,
    HKEY_LONG_RELEASE
} hkey_state_t;

typedef struct {
    hkey_t          key;
    hkey_state_t    state;
} event_hkey_t;

typedef enum {
//  LV_KEY_HOME             = 2,
//  LV_KEY_END              = 3,
    KEY_VOL_LEFT_EDIT       = 4,
    KEY_VOL_RIGHT_EDIT      = 5,
    KEY_VOL_LEFT_SELECT     = 6,
    KEY_VOL_RIGHT_SELECT    = 7,
//  LV_KEY_BACKSPACE        = 8,
//  LV_KEY_NEXT             = 9,
//  LV_KEY_ENTER            = 10,
//  LV_KEY_PREV             = 11,
    HKEY_FINP               = 12
//  LV_KEY_UP               = 17,
//  LV_KEY_DOWN             = 18,
//  LV_KEY_RIGHT            = 19,
//  LV_KEY_LEFT             = 20,
//  LV_KEY_ESC              = 27,
//  LV_KEY_DEL              = 127,
} keys_t;

extern uint32_t EVENT_ROTARY;
extern uint32_t EVENT_KEYPAD;
extern uint32_t EVENT_HKEY;
extern uint32_t EVENT_RADIO_TX;
extern uint32_t EVENT_RADIO_RX;
extern uint32_t EVENT_PANNEL_UPDATE;
extern uint32_t EVENT_MSG_UPDATE;
extern uint32_t EVENT_MSG_LONG_UPDATE;
extern uint32_t EVENT_MSG_TINY_UPDATE;
extern uint32_t EVENT_FREQ_UPDATE;
extern uint32_t EVENT_FT8_MSG;
extern uint32_t EVENT_SSTV_VIS;
extern uint32_t EVENT_SSTV_LINE;
extern uint32_t EVENT_SSTV_FRAME;
extern uint32_t EVENT_GPS;
extern uint32_t EVENT_VOICE_DONE;
extern uint32_t EVENT_BLUETOOTH_MSG;

void event_init();
