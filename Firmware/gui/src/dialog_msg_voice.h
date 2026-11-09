/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include "lvgl/lvgl.h"
#include "dialog.h"

typedef enum {
    MSG_VOICE_OFF = 0,
    MSG_VOICE_RECORD,
    MSG_VOICE_PLAY,
    MSG_VOICE_SEND,
    MSG_VOICE_SEND_CANCEL,
} msg_voice_state_t;

extern dialog_t *dialog_msg_voice;

msg_voice_state_t dialog_msg_voice_get_state();
void dialog_msg_voice_put_audio_samples(float *samples, size_t n);
