/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_demodulator.h"

MFSK_Demodulator::MFSK_Demodulator() {
    Init();
}

MFSK_Demodulator::~MFSK_Demodulator() {
    Free();
}

void MFSK_Demodulator::Init(void) {
    InpTap      = 0;
    SymbolShape = 0;
    FFT_Buff    = 0;
    Spectra[0]  = 0;
    Spectra[1]  = 0;
}

void MFSK_Demodulator::Free(void) {
    free(InpTap);
    InpTap = 0;
    free(SymbolShape);
    SymbolShape = 0;
    free(FFT_Buff);
    FFT_Buff = 0;
    free(Spectra[0]);
    Spectra[0] = 0;
    free(Spectra[1]);
    Spectra[1] = 0;
    FFT.Free();
    History.Free();
}

int MFSK_Demodulator::Preset(MFSK_Parameters *NewParameters) {
    Parameters       = NewParameters;
    SymbolSepar      = Parameters->SymbolSepar;
    SymbolLen        = Parameters->SymbolLen;
    SpectraPerSymbol = Parameters->SpectraPerSymbol;
    InputLen         = SymbolSepar;
    DecodeMargin     = Parameters->RxSyncMargin * Parameters->CarrierSepar;
    WrapMask         = SymbolLen - 1;
    float ShapeScale = 1.0 / SymbolLen;

    if (ReallocArray(&InpTap, SymbolLen) < 0) {
        goto Error;
    }

    ClearArray(InpTap, SymbolLen);
    InpTapPtr = 0;

    if (FFT.Preset(SymbolLen) < 0) {
        goto Error;
    }

    if (ReallocArray(&FFT_Buff, SymbolLen) < 0) {
        goto Error;
    }

    SliceSepar = SymbolSepar / SpectraPerSymbol;

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
            SymbolShape[Time] += Ampl * FFT.Twiddle[Phase].Re;
            Phase += Freq;
            if (Phase >= SymbolLen) {
                Phase -= SymbolLen;
            }
        }
    }
    {
        size_t Time;

        for (Time = 0; Time < SymbolLen; Time++) {
            SymbolShape[Time] *= ShapeScale;
        }
    }

    SpectraLen = SymbolLen / 2;

    if (ReallocArray(&Spectra[0], SpectraLen) < 0) {
        goto Error;
    }

    if (ReallocArray(&Spectra[1], SpectraLen) < 0) {
        goto Error;
    }

    DecodeWidth   = ((Parameters->Carriers - 1) * Parameters->CarrierSepar + 1) + 2 * DecodeMargin;

    History.Len   = (Parameters->RxSyncIntegLen + 2) * Parameters->SpectraPerBlock;
    History.Width = DecodeWidth;

    if (History.Preset() < 0) {
        goto Error;
    }

    History.Clear();

    return 0;

Error:
    Free();
    return -1;
}

void MFSK_Demodulator::Reset(void) {
    History.Clear();
}

float * MFSK_Demodulator::HistoryPtr(int Idx) {
    return History.OffsetPtr(Idx);
}

int MFSK_Demodulator::SlideOneSlice(float *Input) {
    for (size_t InpIdx = 0; InpIdx < SliceSepar; InpIdx++) {
        InpTap[InpTapPtr] = Input[InpIdx];
        InpTapPtr += 1;
        InpTapPtr &= WrapMask;
    }
    return SliceSepar;
}

void MFSK_Demodulator::Process(float *Input) {
    size_t InpIdx, Time, Slice;

    for (InpIdx = 0, Slice = 0; Slice < SpectraPerSymbol; Slice += 2) {
        InpIdx += SlideOneSlice(Input + InpIdx);

        for (Time = 0; Time < SymbolLen; Time++) {
            FFT_Buff[Time].Re = InpTap[InpTapPtr] * SymbolShape[Time];
            InpTapPtr += 1;
            InpTapPtr &= WrapMask;
        }

        InpIdx += SlideOneSlice(Input + InpIdx);

        for (Time = 0; Time < SymbolLen; Time++) {
            FFT_Buff[Time].Im = InpTap[InpTapPtr] * SymbolShape[Time];
            InpTapPtr += 1;
            InpTapPtr &= WrapMask;
        }

        FFT.Process(FFT_Buff);
        FFT.SeparTwoReals(FFT_Buff, Spectra[0], Spectra[1]);

        float *Data0 = History.OffsetPtr(0);
        float *Data1 = History.OffsetPtr(1);

        size_t Idx;
        size_t Freq = Parameters->FirstCarrier - DecodeMargin;

        for (Idx = 0; Idx < DecodeWidth; Idx++, Freq++) {
            Data0[Idx] = Spectra[0][Freq].Energy();
            Data1[Idx] = Spectra[1][Freq].Energy();
        }

        History += 2;
    }
}

int MFSK_Demodulator::PickBlock(float *Spectra, int TimeOffset, int FreqOffset) {
    int SpectraPerBlock = Parameters->SpectraPerBlock;

    if ((TimeOffset > (-SpectraPerBlock)) || ((-TimeOffset) > (int) History.Len)) {
        return -1;
    }

    size_t Carriers     = Parameters->Carriers;
    size_t CarrierSepar = Parameters->CarrierSepar;

    if ((FreqOffset < 0) || ((FreqOffset + (Carriers - 1) * CarrierSepar) >= DecodeWidth)) {
        return -1;
    }

    size_t SymbolsPerBlock = Parameters->SymbolsPerBlock;
    size_t Symbol;

    for (Symbol = 0; Symbol < SymbolsPerBlock; Symbol++, TimeOffset += SpectraPerSymbol) {
        float *Hist = History.OffsetPtr(TimeOffset) + FreqOffset;
        size_t Freq;

        for (Freq = 0; Freq < Carriers; Freq++, Hist += CarrierSepar) {
            (*Spectra++) = (*Hist);
        }
    }

    return 0;
}
