/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "mfsk_parameters.h"
#include "mfsk_softdemodulate.h"
#include "buffer.h"
#include "cmpx.h"
#include "fft.h"
#include "fht.h"
#include "gray.h"
#include "lowpass3.h"
#include "struc.h"
#include "utils.h"

template <class Type> int FitPeak(Type &PeakPos, Type &Peak, Type Left, Type Center, Type Right) {
    Type A = (Right + Left) / 2 - Center;

    if (A >= 0) {
        return -1;
    }
    Type B  = (Right - Left) / 2;

    PeakPos = (-B / (2 * A));
    Peak    = A * PeakPos * PeakPos + B * PeakPos + Center;
    return 1;
}

// =====================================================================

template <class Type> Type Limit(Type X, Type Lower, Type Upper) {
    if (X > Upper) {
        return Upper;
    }
    if (X < Lower) {
        return Lower;
    }
    return X;
}

// =====================================================================

// convert audio from floating point to 16-bit signed

template <class Type>
void ConvertToS16(Type *Input, int16_t *Output, size_t Len, Type Scale = 32768.0) {
    size_t    Idx;
    const int Limit = 32767;

    for (Idx = 0; Idx < Len; Idx++) {
        int Out = (int) floor(Scale * Input[Idx] + 0.5);

        if (Out > Limit) {
            Out = Limit;
        } else if (Out < (-Limit)) {
            Out = (-Limit);
        }
        Output[Idx] = Out;
    }
}

template <class Type>
int ConvertToS16(Seq<Type> &Input, Seq<int16_t> &Output, Type Scale = 32768.0) {
    if (Output.EnsureSpace(Input.Len) < 0) {
        return -1;
    }

    Output.Len = Input.Len;
    ConvertToS16(Input.Elem, Output.Elem, Input.Len, Scale);
    return 0;
}

// the symbol shape described in frequency domain

static const double MFSK_SymbolFreqShape[]  = {+1.0000000000, +2.1373197349, +1.1207588117, -0.0165609232};
static const size_t MFSK_SymbolFreqShapeLen = sizeof(MFSK_SymbolFreqShape) / sizeof(double);

template <class EnergyType, class SymbolType>
void MFSK_SoftModulate(EnergyType *CarrierProb, SymbolType *Symbol, size_t BitsPerSymbol,
                       int UseGrayCode = 1) {
    size_t Carriers = Exp2(BitsPerSymbol);
    size_t Idx;

    for (Idx = 0; Idx < Carriers; Idx++) // loop over carriers
    {
        uint8_t SymbIdx = Idx;

        if (UseGrayCode) {
            SymbIdx = BinaryCode(SymbIdx);
        }

        EnergyType Prob = 1;
        size_t     Bit;
        uint8_t    Mask = 1;

        for (Bit = 0; Bit < BitsPerSymbol; Bit++) {
            EnergyType BitProb = 1.0;

            if (SymbIdx & Mask) {
                BitProb -= Symbol[Bit];
            } else {
                BitProb += Symbol[Bit];
            }
            Prob *= (BitProb / 2);
            Mask <<= 1;
        }
        CarrierProb[Idx] = Prob;
    }
}

// =====================================================================

// A running-box, low pass filter
template <class TapType = float, class OutType = double> class BoxFilter {
public:
    size_t   Len;
    TapType *Tap;
    size_t   Ptr;
    OutType  Output;

    BoxFilter() {
        Tap = 0;
    }

    ~BoxFilter() {
        free(Tap);
    }

    void Free(void) {
        free(Tap);
        Tap = 0;
    }

    int Preset(void) {
        if (ReallocArray(&Tap, Len) < 0) {
            return -1;
        }

        Clear();
        return 0;
    }

    void Clear(void) {
        for (size_t Idx = 0; Idx < Len; Idx++) {
            Tap[Idx] = 0;
        }

        Ptr    = 0;
        Output = 0;
    }

    template <class InpType> void Process(InpType Input) {
        Output -= Tap[Ptr];
        Output += Input;
        Tap[Ptr] = Input;
        Ptr += 1;

        if (Ptr >= Len) {
            Ptr -= Len;
        }
    }
};

// =====================================================================

template <class Type> void PrintBinary(Type Number, size_t Bits) {
    Type Mask = 1;

    Mask <<= (Bits - 1);
    for (; Bits; Bits--) {
        printf("%c", Number & Mask ? '1' : '0');
        Mask >>= 1;
    }
}
