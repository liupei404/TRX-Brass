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

class MFSK_InputProcessor {
public:
    // the user-settable parameters:
    size_t WindowLen;    // spectral analysis (FFT) window length
    float   LimiterLevel; // limiter level (amplitude) to reduce time and frequency localised
                         // interference

public:
    size_t            WrapMask;     // wrap mask for buffer addressing
    float             *InpTap;       // input buffer for analysis window
    size_t            InpTapPtr;
    float             *OutTap;       // output buffer for reconstruction window
    size_t            OutTapPtr;
    float             *WindowShape;  // analysis/reconstruction window shape
    size_t            SliceSepar;   // time separation between analysis/reconstruction slices
    r2FFT<Cmpx<float>> FFT;          // FFT engine
    Cmpx<float>       *FFT_Buff;     // FFT buffer
    size_t            SpectraLen;   // number of spectral points after FFT
    Cmpx<float>       *Spectra[2];   // spectra buffer
    float             *Output;       // (final) output buffer after pulse limiter
    float             *Energy;       // energy vs frequency
    BoxFilter<float>   Filter;       // spectra energy averaging filter

public:
    MFSK_InputProcessor();
    ~MFSK_InputProcessor();

    void Init(void);
    void Free(void);
    void Default(void);
    int Preset(void);
    void Reset(void);
    void LimitSpectraPeaks(Cmpx<float> *Spectra, size_t BoxLen = 64);
    void LimitOutputPeaks(void);
    void AverageEnergy(size_t Len = 32);

    void ProcessSpectra(Cmpx<float> *Spectra);
    void ProcessInpTap(float *Input);
    void ProcessInpTap();
    void ProcessInpWindow_Re(void);
    void ProcessInpWindow_Im(void);
    void ProcessOutWindow_Re(void);
    void ProcessOutWindow_Im(void);
    void ProcessOutTap(float *Output);
    int Process(float *Input);

    int GetOutput(int16_t *Buffer);
};
