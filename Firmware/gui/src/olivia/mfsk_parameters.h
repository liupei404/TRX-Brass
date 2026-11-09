/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>

class MFSK_Parameters {
public:
    // primary parameters
    size_t BitsPerSymbol;    // [Bits]
    size_t Bandwidth;        // [Hz]
    size_t SampleRate;       // [Hz]
    float  LowerBandEdge;    // [Hz]
    size_t RxSyncMargin;     // [MFSK carriers]
    size_t RxSyncIntegLen;   // [FEC Blocks]
    float  RxSyncThreshold;  // [S/N]

    // fixed parameters
    static const size_t   BitsPerCharacter   = 7; // [Bits]
    static const size_t   SymbolsPerBlock    = 1 << (BitsPerCharacter - 1);
    static const size_t   CarrierSepar       = 4; // [FFT bins]
    static const size_t   SpectraPerSymbol   = 4; // [Spectral (FFT) slices]
    static const size_t   SpectraPerBlock    = SpectraPerSymbol * SymbolsPerBlock;
    static const size_t   UseGrayCode        = 1;
    static const size_t   PhaseDiffer        = 1;
    static const size_t   RxSyncSquareEnergy = 1;
    static const size_t   DecodeSquareEnergy = 1;
    static const uint64_t ScramblingCode     = 0xE257E6D0291574ECLL;

    // secondary parameters
    size_t Carriers;
    size_t SymbolSepar;  // [Samples]
    size_t SymbolLen;    // [Samples]
    size_t FirstCarrier; // [FFT bins]

    MFSK_Parameters();

    void  SetTones(uint16_t x);
    void  SetBandWidth(uint16_t x);
    void  SetBandLower(uint16_t x);
    void  SetSyncMargin(uint16_t x);
    void  SetSyncInteg(uint16_t x);
    void  SetSyncThreshold(float x);

    int   Preset(void);

    float BaudRate(void);
    float FFTbinBandwidth(void);
    float CarrierBandwidth(void);
    float TuneMargin(void);
    float BlockPeriod(void);
    float CharactersPerSecond(void);
};
