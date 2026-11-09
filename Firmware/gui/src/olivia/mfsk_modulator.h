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

class MFSK_Modulator {
public:
    MFSK_Parameters *Parameters;

public:
    size_t OutputLen; // output length per transmitted symbol [samples]

private:
    size_t SymbolLen;
    size_t SymbolSepar;

    float *CosineTable; // Cosine table for fast cos/sin calculation
    float *SymbolShape; // the shape of the symbol
    int    SymbolPhase; // the phase of the tone being transmitted
    float *OutTap;      // output tap (buffer)
    size_t TapPtr;
    size_t WrapMask;

public:
    MFSK_Modulator();
    ~MFSK_Modulator();

    void Init(void);
    void Free(void);
    int  Preset(MFSK_Parameters *NewParameters);
    void Send(uint8_t Symbol);

    int  Output(int16_t *Buffer);
    int  Output(float *Buffer);

private:
    void AddSymbol(int Freq, int Phase);
};
