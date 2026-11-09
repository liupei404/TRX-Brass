/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <liquid/liquid.h>

#include "mfsk.h"
#include "mfsk_inputprocessor.h"
#include "mfsk_demodulator.h"
#include "mfsk_synchronizer.h"
#include "mfsk_iterdecoder.h"

class MFSK_Receiver {
public:
    MFSK_Parameters *Parameters;

private:
    MFSK_InputProcessor         InputProcessor; // equalizes the input spectrum and removes coherent interferences
    MFSK_Demodulator            Demodulator;    // spectral (FFT) demodulator
    MFSK_Synchronizer           Synchronizer;   // synchronizer
    MFSK_SoftIterDecoder        Decoder;        // iterative decoder
    FIFO<uint8_t>               Output;         // buffer for decoded characters

public:
    MFSK_Receiver();
    ~MFSK_Receiver();

    void  Init(void);
    void  Free(void);
    int   Preset(MFSK_Parameters *NewParameters);
    void  Reset(void);
    int   WindowLen();

    float SyncSNR(void);
    float FrequencyOffset(void);
    float FrequencyDrift(void);
    float TimeDrift(void);
    float InputSNRdB(void);

    int   GetChar(uint8_t &Char);
    void  ProcessInputBuffer(float *Input);

private:
    void ProcessSymbol(float *Input);
};
