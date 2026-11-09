/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include "mfsk.h"

void MFSK_SoftDemodulate(float *Symbol, float *SpectraEnergy, size_t BitsPerSymbol,
                         size_t CarrierSepar = 1, int UseGrayCode = 1, int SquareEnergy = 1);
