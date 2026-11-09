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
#include "mfsk_encoder.h"
#include "mfsk_modulator.h"

class MFSK_Transmitter {
public:
    MFSK_Parameters *Parameters;
    size_t           MaxOutputLen; // maximum length of the audio batch returned by Output()

private:
    size_t                BitsPerSymbol;
    size_t                SymbolsPerBlock;

    static const int      State_Running = 0x0001;
    static const int      State_StopReq = 0x0010;
    int                   State;
    FIFO<uint8_t>         Input;         // buffer(queue) for the characters to be encoded
    uint8_t               InputBlock[8]; // FEC code block buffer
    FIFO<uint8_t>         Monitor;       // buffer for monitoring the characters being sent
    MFSK_Encoder          Encoder;       // FEC encoder
    size_t                SymbolPtr;
    MFSK_Modulator        Modulator;       // MFSK modulator
    float                *ModulatorOutput; // modulator output
    float                *ConverterOutput; // rate converter output

public:
    MFSK_Transmitter();
    ~MFSK_Transmitter();

    void Init(void);
    void Free(void);
    int  Preset(MFSK_Parameters *NewParameters);
    void Reset(void);
    void Start(void);
    void Stop(void);
    int  Running(void);
    int  PutChar(uint8_t Char);
    int  GetChar(uint8_t &Char);
    int  Output(float *&OutputPtr);
    int  Output(int16_t *Buffer);
};
