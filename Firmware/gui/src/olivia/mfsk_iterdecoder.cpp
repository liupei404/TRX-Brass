/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_iterdecoder.h"
#include "mfsk_softdemodulate.h"

MFSK_SoftIterDecoder::MFSK_SoftIterDecoder() {
    Init();
}

MFSK_SoftIterDecoder::~MFSK_SoftIterDecoder() {
    Free();
}

void MFSK_SoftIterDecoder::Init(void) {
    Input          = 0;
    InputExtrinsic = 0;
    FHT_Codeword   = 0;
    OutputBlock    = 0;
}

void MFSK_SoftIterDecoder::Free(void) {
    free(Input);
    Input = 0;
    free(InputExtrinsic);
    InputExtrinsic = 0;
    free(FHT_Codeword);
    FHT_Codeword = 0;
    free(OutputBlock);
    OutputBlock = 0;
}

int MFSK_SoftIterDecoder::Preset(MFSK_Parameters *NewParameters) {
    Parameters       = NewParameters;
    BitsPerSymbol    = Parameters->BitsPerSymbol;
    BitsPerCharacter = Parameters->BitsPerCharacter;
    Symbols          = Parameters->Carriers;
    SymbolsPerBlock  = Parameters->SymbolsPerBlock;

    if (ReallocArray(&Input, SymbolsPerBlock * Symbols) < 0) {
        goto Error;
    }

    if (ReallocArray(&InputExtrinsic, SymbolsPerBlock * Symbols) < 0) {
        goto Error;
    }

    if (ReallocArray(&FHT_Codeword, SymbolsPerBlock * BitsPerSymbol) < 0) {
        goto Error;
    }

    if (ReallocArray(&OutputBlock, BitsPerSymbol) < 0) {
        goto Error;
    }

    return 0;
Error:
    Free();
    return -1;
}

int MFSK_SoftIterDecoder::NormalizeSum(float *Data, size_t Len, float Norm) {
    size_t Idx;
    float  Sum = 0;

    for (Idx = 0; Idx < Len; Idx++) {
        Sum += Data[Idx];
    }

    if (Sum <= 0) {
        return -1;
    }

    float Corr = Norm / Sum;

    for (Idx = 0; Idx < Len; Idx++) {
        Data[Idx] *= Corr;
    }

    return 0;
}

int MFSK_SoftIterDecoder::NormalizeAbsSum(float *Data, size_t Len, float Norm) {
    size_t Idx;
    float  Sum = 0;

    for (Idx = 0; Idx < Len; Idx++) {
        Sum += fabs(Data[Idx]);
    }

    if (Sum <= 0) {
        return -1;
    }

    float Corr = Norm / Sum;

    for (Idx = 0; Idx < Len; Idx++) {
        Data[Idx] *= Corr;
    }

    return 0;
}

int MFSK_SoftIterDecoder::ThirdPower(float *Data, size_t Len) {
    for (size_t Idx = 0; Idx < Len; Idx++) {
        float Square = Data[Idx];

        // Square=Square*Square;
        Square = fabs(Square); // <= works better (in simulations)
        Data[Idx] *= Square;
    }

    return 0;
}

void MFSK_SoftIterDecoder::ScrambleCodeword(float *CodeWord, size_t ScrambleIdx) {
    size_t Idx;
    size_t CodeWrap = (SymbolsPerBlock - 1);

    ScrambleIdx &= CodeWrap;

    for (Idx = 0; Idx < SymbolsPerBlock; Idx++) {
        uint64_t CodeMask = 1;

        CodeMask <<= ScrambleIdx;

        if (Parameters->ScramblingCode & CodeMask) {
            CodeWord[Idx] = (-CodeWord[Idx]);
        }

        ScrambleIdx += 1;
        ScrambleIdx &= CodeWrap;
    }
}

uint8_t MFSK_SoftIterDecoder::DecodeChar(float *FHT_Buffer) {
    size_t TimeBit;
    float  PeakAbs     = 0;
    float  Peak        = 0;
    size_t PeakPos     = 0;
    float  NoiseEnergy = 0;

    for (TimeBit = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
        float Signal = FHT_Buffer[TimeBit];

        NoiseEnergy += Signal * Signal;

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

    float SignalEnergy = Peak * Peak;

    NoiseEnergy -= SignalEnergy;
    SignalEnergy -= NoiseEnergy / (SymbolsPerBlock - 1);
    NoiseEnergy *= (float) SymbolsPerBlock / (SymbolsPerBlock - 1);

    FEC_SignalEnergy += SignalEnergy;
    FEC_NoiseEnergy += NoiseEnergy;

    return Char;
}

void MFSK_SoftIterDecoder::Process(size_t MaxIter) {
    size_t TimeBit;
    size_t Bit;
    size_t Freq;
    size_t InpIdx;
    size_t BlockIdx;
    size_t InputSize = Symbols * SymbolsPerBlock;
    size_t BlockSize = BitsPerSymbol * SymbolsPerBlock;

    for (InpIdx = 0; InpIdx < InputSize; InpIdx++) {
        InputExtrinsic[InpIdx] = 1.0 / Symbols;
    }

    for (; MaxIter; MaxIter--) {
        int SquareEnergy = Parameters->DecodeSquareEnergy;

        for (InpIdx = 0; InpIdx < InputSize; InpIdx++) {
            float InputEnergy = Input[InpIdx];

            if (SquareEnergy) {
                InputEnergy *= InputEnergy;
            }
            InputExtrinsic[InpIdx] *= InputEnergy;
        }

        float  SymbolBit[BitsPerSymbol];
        size_t Rotate = 0;

        for (TimeBit = 0, InpIdx = 0; TimeBit < SymbolsPerBlock; TimeBit++, InpIdx += Symbols) {
            MFSK_SoftDemodulate(SymbolBit, InputExtrinsic + InpIdx, BitsPerSymbol, 1, Parameters->UseGrayCode, 0);

            BlockIdx = TimeBit + Rotate * SymbolsPerBlock;

            for (Bit = 0; Bit < BitsPerSymbol; Bit++) {
                FHT_Codeword[BlockIdx] = SymbolBit[Bit];
                BlockIdx += SymbolsPerBlock;
                if (BlockIdx >= BlockSize) {
                    BlockIdx -= BlockSize;
                }
            }

            if (Rotate > 0) {
                Rotate -= 1;
            } else {
                Rotate += (BitsPerSymbol - 1);
            }
        }

        FEC_SignalEnergy = 0;
        FEC_NoiseEnergy  = 0;

        for (Bit = 0, BlockIdx = 0; Bit < BitsPerSymbol; Bit++, BlockIdx += SymbolsPerBlock) {
            ScrambleCodeword(FHT_Codeword + BlockIdx, 13 * Bit);
            FHT(FHT_Codeword + BlockIdx);

            uint8_t Char     = DecodeChar(FHT_Codeword + BlockIdx);
            OutputBlock[Bit] = Char;

            ThirdPower(FHT_Codeword + BlockIdx, SymbolsPerBlock);
            NormalizeAbsSum(FHT_Codeword + BlockIdx, SymbolsPerBlock, 1.0);

            IFHT_float(FHT_Codeword + BlockIdx);
            ScrambleCodeword(FHT_Codeword + BlockIdx, 13 * Bit);
        }

        Rotate = 0;

        for (TimeBit = 0, InpIdx = 0; TimeBit < SymbolsPerBlock; TimeBit++, InpIdx += Symbols) {
            BlockIdx = TimeBit + Rotate * SymbolsPerBlock;

            for (Bit = 0; Bit < BitsPerSymbol; Bit++) {
                SymbolBit[Bit] = FHT_Codeword[BlockIdx];
                BlockIdx += SymbolsPerBlock;
                if (BlockIdx >= BlockSize) {
                    BlockIdx -= BlockSize;
                }
            }

            MFSK_SoftModulate(InputExtrinsic + InpIdx, SymbolBit, BitsPerSymbol, Parameters->UseGrayCode);
            if (Rotate > 0) {
                Rotate -= 1;
            } else {
                Rotate += (BitsPerSymbol - 1);
            }
        }

        Input_SignalEnergy = 0;
        Input_NoiseEnergy  = 0;

        for (TimeBit = 0, InpIdx = 0; TimeBit < SymbolsPerBlock; TimeBit++) {
            for (Freq = 0; Freq < Symbols; Freq++, InpIdx++) {
                float Energy  = Input[InpIdx];
                float SigProb = InputExtrinsic[InpIdx];

                Input_SignalEnergy += SigProb * Energy;
                Input_NoiseEnergy += (1 - SigProb) * Energy;
            }
        }

        Input_SignalEnergy -= Input_NoiseEnergy / (Symbols - 1);
        Input_NoiseEnergy *= (float) Symbols / (Symbols - 1);
    }
}

float MFSK_SoftIterDecoder::InputSNRdB(void) {
    return 10 * log(Input_SignalEnergy / Input_NoiseEnergy) / log(10);
}

void MFSK_SoftIterDecoder::PrintSNR(void) {
    size_t Bit;

    printf("Input: %+5.1f dB", 10 * log(Input_SignalEnergy / Input_NoiseEnergy) / log(10));
    printf(" : ");

    for (Bit = 0; Bit < BitsPerSymbol; Bit++) {
        char Char = OutputBlock[Bit];

        printf("%c", Char > ' ' ? Char : ' ');
    }

    printf("\n");
}

int MFSK_SoftIterDecoder::WriteOutputBlock(FIFO<uint8_t> &Output) {
    int    Written = 0;

    for (size_t Bit = 0; Bit < BitsPerSymbol; Bit++) {
        uint8_t Char  = OutputBlock[Bit];
        int     Error = Output.Write(Char);

        if (Error < 0) {
            break;
        }

        Written += Error;
    }
    return Written;
}
