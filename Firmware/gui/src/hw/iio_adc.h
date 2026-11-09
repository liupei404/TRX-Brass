/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#define IIO_ADC_RATE_SPS 3300

struct iio_device;
struct iio_channel;

int iio_adc_set_sampling_frequency(struct iio_channel *channel);
int iio_adc_start(struct iio_device *dev, struct iio_channel *fwd, struct iio_channel *rev, void (*publish)(int, int));
void iio_adc_stop(void);
