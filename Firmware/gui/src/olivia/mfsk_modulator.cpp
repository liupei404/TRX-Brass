/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_modulator.h"

MFSK_Modulator::MFSK_Modulator() {
    Init();
}

MFSK_Modulator::~MFSK_Modulator() {
    Free();
}

void MFSK_Modulator::Init(void) {
    CosineTable = 0;
    SymbolShape = 0;
    OutTap      = 0;
}

void MFSK_Modulator::Free(void) {
    free(CosineTable);
    CosineTable = 0;
    free(SymbolShape);
    SymbolShape = 0;
    free(OutTap);
    OutTap = 0;
}

int MFSK_Modulator::Preset(MFSK_Parameters *NewParameters) {
    Parameters  = NewParameters;

    SymbolLen   = Parameters->SymbolLen;
    SymbolSepar = Parameters->SymbolSepar;

    size_t Idx;

    if (ReallocArray(&CosineTable, SymbolLen) < 0) {
        goto Error;
    }

    for (Idx = 0; Idx < SymbolLen; Idx++) {
        CosineTable[Idx] = cos((2 * M_PI * Idx) / SymbolLen);
    }

    if (ReallocArray(&SymbolShape, SymbolLen) < 0) {
        goto Error;
    }

    {
        size_t Time;
        double Ampl = MFSK_SymbolFreqShape[0];

        for (Time = 0; Time < SymbolLen; Time++) {
            SymbolShape[Time] = Ampl;
        }
    }
    size_t Freq;

    for (Freq = 1; Freq < MFSK_SymbolFreqShapeLen; Freq++) {
        size_t Time;
        double Ampl = MFSK_SymbolFreqShape[Freq];

        if (Freq & 1) {
            Ampl = (-Ampl);
        }

        size_t Phase = 0;

        for (Time = 0; Time < SymbolLen; Time++) {
            SymbolShape[Time] += Ampl * CosineTable[Phase];
            Phase += Freq;

            if (Phase >= SymbolLen) {
                Phase -= SymbolLen;
            }
        }
    }
    {
        size_t Time;
        double Scale = 1.0 / (2 * Parameters->CarrierSepar);

        for (Time = 0; Time < SymbolLen; Time++) {
            SymbolShape[Time] *= Scale;
        }
    }

    if (ReallocArray(&OutTap, SymbolLen) < 0) {
        goto Error;
    }

    for (Idx = 0; Idx < SymbolLen; Idx++) {
        OutTap[Idx] = 0;
    }

    TapPtr      = 0;
    WrapMask    = SymbolLen - 1;
    SymbolPhase = 0;
    OutputLen   = SymbolSepar;

    return 0;

Error:
    Free();
    return -1;
}

void MFSK_Modulator::Send(uint8_t Symbol) {
    if (Parameters->UseGrayCode) {
        Symbol = GrayCode(Symbol);
    }

    int SymbolFreq = Parameters->FirstCarrier + Parameters->CarrierSepar * Symbol;
    int TimeShift  = SymbolSepar / 2 - SymbolLen / 2;

    SymbolPhase += SymbolFreq * TimeShift;
    SymbolPhase &= WrapMask;

    AddSymbol(SymbolFreq, SymbolPhase);

    TimeShift = SymbolSepar / 2 + SymbolLen / 2;
    SymbolPhase += SymbolFreq * TimeShift;
    SymbolPhase &= WrapMask;

    if (Parameters->PhaseDiffer) {
        int PhaseShift = SymbolLen / 4;

        if (rand() & 1) {
            PhaseShift = (-PhaseShift);
        }
        SymbolPhase += PhaseShift;
    }

    SymbolPhase &= WrapMask;
}

int MFSK_Modulator::Output(int16_t *Buffer) {
    const float   Scale = 32768.0;
    const int32_t Limit = 0x7FFF;

    for (size_t Idx = 0; Idx < SymbolSepar; Idx++) {
        float Ampl = OutTap[TapPtr];

        Ampl *= Scale;
        int32_t Out = (int32_t) floor(Ampl + 0.5);

        if (Out > Limit) {
            Out = Limit;
        } else if (Out < (-Limit)) {
            Out = (-Limit);
        }

        Buffer[Idx]    = (int16_t) Out;
        OutTap[TapPtr] = 0;
        TapPtr += 1;
        TapPtr &= WrapMask;
    }

    return SymbolSepar;
}

int MFSK_Modulator::Output(float *Buffer) {
    for (size_t Idx = 0; Idx < SymbolSepar; Idx++) {
        Buffer[Idx]    = OutTap[TapPtr];
        OutTap[TapPtr] = 0;
        TapPtr += 1;
        TapPtr &= WrapMask;
    }

    return SymbolSepar;
}

void MFSK_Modulator::AddSymbol(int Freq, int Phase) {
    for (size_t Time = 0; Time < SymbolLen; Time++) {
        OutTap[TapPtr] += CosineTable[Phase] * SymbolShape[Time];
        Phase += Freq;
        Phase &= WrapMask;
        TapPtr += 1;
        TapPtr &= WrapMask;
    }
}
