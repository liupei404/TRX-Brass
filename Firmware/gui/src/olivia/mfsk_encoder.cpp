/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_encoder.h"

MFSK_Encoder::MFSK_Encoder() {
    Init();
    Default();
}

MFSK_Encoder::~MFSK_Encoder() {
    Free();
}

void MFSK_Encoder::Default(void) {
    BitsPerSymbol    = 5;
    BitsPerCharacter = 7;
}

void MFSK_Encoder::Init(void) {
    FHT_Buffer  = 0;
    OutputBlock = 0;
}

void MFSK_Encoder::Free(void) {
    free(FHT_Buffer);
    FHT_Buffer = 0;
    free(OutputBlock);
    OutputBlock = 0;
}

int MFSK_Encoder::Preset(void) {
    Symbols         = 1 << BitsPerSymbol;
    SymbolsPerBlock = Exp2(BitsPerCharacter - 1);

    if (ReallocArray(&FHT_Buffer, SymbolsPerBlock) < 0) {
        goto Error;
    }

    if (ReallocArray(&OutputBlock, SymbolsPerBlock) < 0) {
        goto Error;
    }

    return 0;
Error:
    Free();
    return -1;
}

void MFSK_Encoder::EncodeCharacter(uint8_t Char) {
    size_t  TimeBit;
    uint8_t Mask = (SymbolsPerBlock << 1) - 1;

    Char &= Mask;

    for (TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
        FHT_Buffer[TimeBit] = 0;
    }

    if (Char < SymbolsPerBlock) {
        FHT_Buffer[Char] = 1;
    } else {
        FHT_Buffer[Char - SymbolsPerBlock] = (-1);
    }

    IFHT_8(FHT_Buffer);
}

void MFSK_Encoder::ScrambleFHT(size_t CodeOffset) {
    size_t TimeBit;
    size_t CodeWrap = (SymbolsPerBlock - 1);
    size_t CodeBit  = CodeOffset & CodeWrap;

    for (TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
        uint64_t CodeMask = 1;

        CodeMask <<= CodeBit;

        if (ScramblingCode & CodeMask) {
            FHT_Buffer[TimeBit] = (-FHT_Buffer[TimeBit]);
        }

        CodeBit += 1;
        CodeBit &= CodeWrap;
    }
}

void MFSK_Encoder::EncodeBlock(uint8_t *InputBlock) {
    size_t FreqBit, TimeBit;

    for (TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
        OutputBlock[TimeBit] = 0;
    }

    for (FreqBit = 0; FreqBit < BitsPerSymbol; FreqBit++) {
        EncodeCharacter(InputBlock[FreqBit]);
        ScrambleFHT(FreqBit * 13);
        size_t Rotate = 0;

        for (TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
            if (FHT_Buffer[TimeBit] < 0) {
                size_t Bit = FreqBit + Rotate;

                if (Bit >= BitsPerSymbol) {
                    Bit -= BitsPerSymbol;
                }
                uint8_t Mask = 1;

                Mask <<= Bit;
                OutputBlock[TimeBit] |= Mask;
            }

            Rotate += 1;

            if (Rotate >= BitsPerSymbol) {
                Rotate -= BitsPerSymbol;
            }
        }
    }
}

void MFSK_Encoder::PrintOutputBlock(void) {
    for (size_t TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
        printf("%2d: ", TimeBit);
        PrintBinary(OutputBlock[TimeBit], BitsPerSymbol);
        printf("\n");
    }
}
