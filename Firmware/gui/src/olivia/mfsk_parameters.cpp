/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mfsk_parameters.h"
#include "utils.h"

MFSK_Parameters::MFSK_Parameters() {
    BitsPerSymbol    = 5;
    SampleRate       = 8000;
    Bandwidth        = 1000;
    LowerBandEdge    = SampleRate / 16;
    RxSyncIntegLen   = 4;
    RxSyncMargin     = 4;
    RxSyncThreshold  = 3.0;
}

void MFSK_Parameters::SetTones(uint16_t x) {
    BitsPerSymbol = Log2(x);
}

void MFSK_Parameters::SetBandWidth(uint16_t x) {
    Bandwidth = x;
}

void MFSK_Parameters::SetBandLower(uint16_t x) {
    LowerBandEdge = x;
}

void MFSK_Parameters::SetSyncMargin(uint16_t x) {
    RxSyncMargin = x;
}

void MFSK_Parameters::SetSyncInteg(uint16_t x) {
    RxSyncIntegLen = x;
}

void MFSK_Parameters::SetSyncThreshold(float x) {
    RxSyncThreshold = x;
}

int MFSK_Parameters::Preset(void) {
    if (BitsPerSymbol > 8) {
        BitsPerSymbol = 8;
    } else if (BitsPerSymbol < 1) {
        BitsPerSymbol = 1;
    }

    Carriers            = Exp2(BitsPerSymbol);

    size_t MinBandwidth = SampleRate / 64;
    size_t MaxBandwidth = SampleRate / 4;

    if (Bandwidth < MinBandwidth) {
        Bandwidth = MinBandwidth;
    } else if (Bandwidth > MaxBandwidth) {
        Bandwidth = MaxBandwidth;
    }

    Bandwidth   = MinBandwidth * Exp2(Log2(Bandwidth / MinBandwidth));
    SymbolSepar = (SampleRate / Bandwidth) * Carriers;
    SymbolLen   = SymbolSepar * CarrierSepar;

    FirstCarrier =
        (size_t) floor((LowerBandEdge / SampleRate) * SymbolLen + 0.5) + (CarrierSepar / 2);

    if ((FirstCarrier + Carriers * CarrierSepar) >= (SymbolLen / 2)) {
        FirstCarrier = (SymbolLen / 2) - Carriers * CarrierSepar;
    }

    LowerBandEdge = (float) (FirstCarrier - CarrierSepar / 2) / SymbolLen;

    if (RxSyncMargin > (FirstCarrier / CarrierSepar)) {
        RxSyncMargin = (FirstCarrier / CarrierSepar);
    }

    return 0;
}

float MFSK_Parameters::BaudRate(void) {
    return (float) SampleRate / SymbolSepar;
}

float MFSK_Parameters::FFTbinBandwidth(void) {
    return (float) SampleRate / SymbolLen;
}

float MFSK_Parameters::CarrierBandwidth(void) {
    return (float) SampleRate / SymbolLen * CarrierSepar;
}

float MFSK_Parameters::TuneMargin(void) {
    return CarrierBandwidth() * RxSyncMargin;
}

float MFSK_Parameters::BlockPeriod(void) {
    return (SymbolsPerBlock * SymbolSepar) / (float) SampleRate;
}

float MFSK_Parameters::CharactersPerSecond(void) {
    return BitsPerSymbol * (float) SampleRate / (SymbolsPerBlock * SymbolSepar);
}
