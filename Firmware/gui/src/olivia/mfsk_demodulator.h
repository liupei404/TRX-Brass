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

class MFSK_Demodulator {
public:
    MFSK_Parameters *Parameters;
    size_t           InputLen; // input must be provided in batches of that length [samples]

private:
    size_t                SymbolSepar;
    size_t                SymbolLen;
    size_t                SpectraPerSymbol;
    size_t                DecodeMargin; // frequency margin for decoding the signal [FFT bins]
    size_t                DecodeWidth;  // Spectra width                            [FFT bins]
    size_t                SliceSepar;   // time separation between samples          [samples]
    size_t                WrapMask;     // wrapping mask for the FFT window
    float                *InpTap;       // input buffer
    size_t                InpTapPtr;
    float                *SymbolShape; // the shape of the symbol and the FFT window
    r2FFT<Cmpx<float>>    FFT;         // FFT engine
    Cmpx<float>          *FFT_Buff;    // FFT buffer
    size_t                SpectraLen;  // number of spectral points per FFT
    Cmpx<float>          *Spectra[2];  // 2 buffers for FFT spectra
    CircularBuffer<float> History;     // Spectra history

public:
    MFSK_Demodulator();
    ~MFSK_Demodulator();

    void   Init(void);
    void   Free(void);
    int    Preset(MFSK_Parameters *NewParameters);
    void   Reset(void);

    float *HistoryPtr(int Idx);
    int    SlideOneSlice(float *Input);
    void   Process(float *Input);
    int    PickBlock(float *Spectra, int TimeOffset, int FreqOffset);
};
