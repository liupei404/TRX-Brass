/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_softdecoder.h"

MFSK_SoftDecoder::MFSK_SoftDecoder() {
    Init();
}

MFSK_SoftDecoder::~MFSK_SoftDecoder() {
    Free();
}

void MFSK_SoftDecoder::Init(void) {
    InputBuffer = 0;
    FHT_Buffer  = 0;
    OutputBlock = 0;
}

void MFSK_SoftDecoder::Free(void) {
    free(InputBuffer);
    InputBuffer = 0;
    free(FHT_Buffer);
    FHT_Buffer = 0;
    free(OutputBlock);
    OutputBlock = 0;
}

void MFSK_SoftDecoder::Reset(void) {
    for (size_t Idx = 0; Idx < InputBufferLen; Idx++) {
        InputBuffer[Idx] = 0;
    }

    InputPtr = 0;
}

int MFSK_SoftDecoder::Preset(MFSK_Parameters *NewParameters) {
    Parameters       = NewParameters;
    BitsPerSymbol    = Parameters->BitsPerSymbol;
    SymbolsPerBlock  = Parameters->SymbolsPerBlock;
    SpectraPerSymbol = Parameters->SpectraPerSymbol;
    InputBufferLen   = SymbolsPerBlock * SpectraPerSymbol * BitsPerSymbol;

    if (ReallocArray(&InputBuffer, InputBufferLen) < 0) {
        goto Error;
    }

    if (ReallocArray(&FHT_Buffer, SymbolsPerBlock) < 0) {
        goto Error;
    }

    if (ReallocArray(&OutputBlock, BitsPerSymbol) < 0) {
        goto Error;
    }

    Reset();

    return 0;
Error:
    Free();
    return -1;
}

void MFSK_SoftDecoder::SpectralInput(float *SpectraEnergy) {
    MFSK_SoftDemodulate(InputBuffer + InputPtr, SpectraEnergy, BitsPerSymbol, Parameters->CarrierSepar,
                        Parameters->UseGrayCode, Parameters->RxSyncSquareEnergy);
    InputPtr += BitsPerSymbol;

    if (InputPtr >= InputBufferLen) {
        InputPtr -= InputBufferLen;
    }
}

void MFSK_SoftDecoder::Input(float *Symbol) {
    for (size_t FreqBit = 0; FreqBit < BitsPerSymbol; FreqBit++) {
        InputBuffer[InputPtr] = Symbol[FreqBit];
        InputPtr += 1;
    }

    if (InputPtr >= InputBufferLen) {
        InputPtr -= InputBufferLen;
    }
}

void MFSK_SoftDecoder::DecodeCharacter(size_t FreqBit) {
    size_t TimeBit;
    size_t Ptr      = InputPtr;
    size_t Rotate   = FreqBit;
    size_t CodeWrap = (SymbolsPerBlock - 1);
    size_t CodeBit  = FreqBit * 13;

    CodeBit &= CodeWrap;

    for (TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
        float    Bit      = InputBuffer[Ptr + Rotate];
        uint64_t CodeMask = 1;

        CodeMask <<= CodeBit;

        if (Parameters->ScramblingCode & CodeMask) {
            Bit = (-Bit);
        }

        FHT_Buffer[TimeBit] = Bit;

        CodeBit += 1;
        CodeBit &= CodeWrap;
        Rotate += 1;

        if (Rotate >= BitsPerSymbol) {
            Rotate -= BitsPerSymbol;
        }

        Ptr += (BitsPerSymbol * SpectraPerSymbol);

        if (Ptr >= InputBufferLen) {
            Ptr -= InputBufferLen;
        }
    }

    FHT(FHT_Buffer);

    float  Peak    = 0;
    float  PeakAbs = 0;
    size_t PeakPos = 0;
    float  SqrSum  = 0;

    for (TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
        float Signal = FHT_Buffer[TimeBit];

        SqrSum += Signal * Signal;

        if (fabs(Signal) > PeakAbs) {
            Peak    = Signal;
            PeakAbs = fabs(Peak);
            PeakPos = TimeBit;
        }
    }

    uint8_t Char = PeakPos;

    if (Peak < 0) {
        Char += SymbolsPerBlock;
    }

    SqrSum -= Peak * Peak;

    OutputBlock[FreqBit] = Char;
    NoiseEnergy += (float) SqrSum / (SymbolsPerBlock - 1);
    Signal += PeakAbs;
}

void MFSK_SoftDecoder::Process(void) {
    size_t FreqBit;

    Signal      = 0;
    NoiseEnergy = 0;

    for (FreqBit = 0; FreqBit < BitsPerSymbol; FreqBit++) {
        DecodeCharacter(FreqBit);
    }

    Signal /= BitsPerSymbol;
    NoiseEnergy /= BitsPerSymbol;
}

size_t MFSK_SoftDecoder::Output(uint8_t *Buffer) {
    size_t FreqBit;

    for (FreqBit = 0; FreqBit < BitsPerSymbol; FreqBit++) {
        Buffer[FreqBit] = OutputBlock[FreqBit];
    }

    return BitsPerSymbol;
}

void MFSK_SoftDecoder::PrintOutputBlock(FILE *File) {
    size_t FreqBit;

    fprintf(File, "'");

    for (FreqBit = 0; FreqBit < BitsPerSymbol; FreqBit++) {
        uint8_t Char = OutputBlock[FreqBit];

        fprintf(File, "%c", (Char >= ' ') && (Char < 127) ? Char : ' ');
    }

    fprintf(File, "', S/N = %5.1f/%4.1f", Signal, sqrt(NoiseEnergy));

    if (NoiseEnergy > 0) {
        fprintf(File, " = %5.1f", Signal / sqrt(NoiseEnergy));
    }

    fprintf(File, "\n");
}
