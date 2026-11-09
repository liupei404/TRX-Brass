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

class MFSK_SoftIterDecoder {
public:
    MFSK_Parameters *Parameters;
    float           *Input; // demodulated spectra energies / tone probabilities
    float            Input_SignalEnergy;
    float            Input_NoiseEnergy;
    float            FEC_SignalEnergy;
    float            FEC_NoiseEnergy;
    uint8_t         *OutputBlock;

private:
    size_t BitsPerSymbol;
    size_t BitsPerCharacter;
    size_t Symbols;
    size_t SymbolsPerBlock;
    float *InputExtrinsic; // extrinsic input information fed back from the decoder
    float *FHT_Codeword;   // FHT codewords to be decoded by FHT

public:
    MFSK_SoftIterDecoder();
    ~MFSK_SoftIterDecoder();

    void    Init(void);
    void    Free(void);
    int     Preset(MFSK_Parameters *NewParameters);
    int     NormalizeSum(float *Data, size_t Len, float Norm = 1.0);
    int     NormalizeAbsSum(float *Data, size_t Len, float Norm = 1.0);
    int     ThirdPower(float *Data, size_t Len);
    void    ScrambleCodeword(float *CodeWord, size_t ScrambleIdx);
    uint8_t DecodeChar(float *FHT_Buffer);
    void    Process(size_t MaxIter = 4);
    float   InputSNRdB(void);
    void    PrintSNR(void);
    int     WriteOutputBlock(FIFO<uint8_t> &Output);
};
