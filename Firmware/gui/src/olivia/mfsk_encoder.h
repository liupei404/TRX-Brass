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

class MFSK_Encoder {
public:
    // parameters to be set before calling Preset()

    size_t   BitsPerSymbol;    // number of bits per MFSK symbol (default is 5 bits thus 32 possible symbols)
    size_t   BitsPerCharacter; // number of bits per transmitted character (default is 7 bits for ASCII code)
    size_t   Symbols;          // number of possible MFSK symbols
    size_t   SymbolsPerBlock;  // number of MFSK symbols per FEC block
    uint8_t *OutputBlock;      // encoded block is stored here

private:
    static const uint64_t ScramblingCode = 0xE257E6D0291574ECLL;
    int8_t               *FHT_Buffer; // temporary buffer for (inverse) Fast Hadamard Transform

public:
    MFSK_Encoder();
    ~MFSK_Encoder();

    void Default(void);
    void Init(void);
    void Free(void);
    int  Preset(void);
    void EncodeCharacter(uint8_t Char);
    void ScrambleFHT(size_t CodeOffset = 0);
    void EncodeBlock(uint8_t *InputBlock);

    void PrintOutputBlock(void);
};
