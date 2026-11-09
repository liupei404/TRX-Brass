/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AUDIO_PLAY_RATE     (44100)
#define AUDIO_CAPTURE_RATE  (44100)

typedef struct {
    char    *name;
    char    *description;
} audio_device_t;

typedef struct {
    audio_device_t  *items;
    size_t          count;
} audio_device_list_t;

void audio_init();

int audio_play(int16_t *buf, size_t samples);
void audio_play_wait();

int audio_adc_play(int16_t *buf, size_t samples);
bool audio_get_devices(bool capture, audio_device_list_t *list);
void audio_free_devices(audio_device_list_t *list);
bool audio_set_device(bool capture, const char *name);
