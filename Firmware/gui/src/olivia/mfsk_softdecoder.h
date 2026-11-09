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

class MFSK_SoftDecoder {
public:
    MFSK_Parameters *Parameters;
    uint8_t         *OutputBlock;
    float            Signal, NoiseEnergy;

private:
    size_t BitsPerSymbol;
    size_t SymbolsPerBlock;
    size_t SpectraPerSymbol;
    size_t InputBufferLen;
    float *InputBuffer;
    size_t InputPtr;
    float *FHT_Buffer;

public:
    MFSK_SoftDecoder();
    ~MFSK_SoftDecoder();

    void   Init(void);
    void   Free(void);
    void   Reset(void);
    int    Preset(MFSK_Parameters *NewParameters);
    void   SpectralInput(float *SpectraEnergy);
    void   Input(float *Symbol);
    void   DecodeCharacter(size_t FreqBit);
    void   Process(void);
    size_t Output(uint8_t *Buffer);
    void   PrintOutputBlock(FILE *File = stdout);
};
