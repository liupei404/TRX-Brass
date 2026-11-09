/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_synchronizer.h"

MFSK_Synchronizer::MFSK_Synchronizer() {
    Init();
}

MFSK_Synchronizer::~MFSK_Synchronizer() {
    Free();
}

void MFSK_Synchronizer::Init(void) {
    Decoder = 0;
}

void MFSK_Synchronizer::Free(void) {
    if (Decoder) {
        for (size_t Idx = 0; Idx < FreqOffsets; Idx++) {
            Decoder[Idx].Free();
        }

        free(Decoder);
        Decoder = 0;
    }
    SyncSignal.Free();
    SyncNoiseEnergy.Free();
}

int MFSK_Synchronizer::Preset(MFSK_Parameters *NewParameters) {
    Parameters = NewParameters;

    size_t Idx;

    if (Decoder) {
        for (Idx = 0; Idx < FreqOffsets; Idx++) {
            Decoder[Idx].Free();
        }
    }

    FreqOffsets = 2 * Parameters->RxSyncMargin * Parameters->CarrierSepar + 1;
    BlockPhases = Parameters->SpectraPerSymbol * Parameters->SymbolsPerBlock;

    if (ReallocArray(&Decoder, FreqOffsets) < 0) {
        goto Error;
    }

    for (Idx = 0; Idx < FreqOffsets; Idx++) {
        Decoder[Idx].Init();
    }

    for (Idx = 0; Idx < FreqOffsets; Idx++) {
        if (Decoder[Idx].Preset(Parameters) < 0) {
            goto Error;
        }
    }

    SyncSignal.Width = FreqOffsets;
    SyncSignal.Len   = BlockPhases;

    if (SyncSignal.Preset() < 0) {
        goto Error;
    }

    SyncNoiseEnergy.Width = FreqOffsets;
    SyncNoiseEnergy.Len   = BlockPhases;

    if (SyncNoiseEnergy.Preset() < 0) {
        goto Error;
    }

    SyncFilterWeight = 1.0 / Parameters->RxSyncIntegLen;

    Reset();

    return 0;

Error:
    Free();
    return -1;
}

void MFSK_Synchronizer::Reset(void) {
    for (size_t Idx = 0; Idx < FreqOffsets; Idx++) {
        Decoder[Idx].Reset();
    }

    SyncSignal.Clear();
    SyncNoiseEnergy.Clear();

    BlockPhase         = 0;

    SyncBestSignal     = 0;
    SyncBestBlockPhase = 0;
    SyncBestFreqOffset = 0;
    SyncSNR            = 0;
    DecodeReference    = (-BlockPhases / 2);

    PreciseFreqOffset  = 0;
    PreciseBlockPhase  = 0;
    StableLock         = 0;
    FreqDrift          = 0;
    TimeDrift          = 0;
}

void MFSK_Synchronizer::Process(float *Spectra) {
    size_t                          Offset;
    MFSK_SoftDecoder               *DecoderPtr     = Decoder;
    LowPass3_Filter<float>         *SignalPtr      = SyncSignal[BlockPhase];
    LowPass3_Filter<float>         *NoiseEnergyPtr = SyncNoiseEnergy[BlockPhase];

    // printf("%3d:",BlockPhase);
    float  BestSliceSignal = 0;
    size_t BestSliceOffset = 0;

    for (Offset = 0; Offset < FreqOffsets; Offset++) {
        DecoderPtr->SpectralInput(Spectra + Offset);
        DecoderPtr->Process();
        float NoiseEnergy = DecoderPtr->NoiseEnergy;
        float Signal      = DecoderPtr->Signal;

        // printf(" %4.1f",Signal);

        NoiseEnergyPtr->Process(NoiseEnergy, SyncFilterWeight);
        SignalPtr->Process(Signal, SyncFilterWeight);
        Signal = SignalPtr->Output;

        // printf("/%4.1f",Signal);

        if (Signal > BestSliceSignal) {
            BestSliceSignal = Signal;
            BestSliceOffset = Offset;
        }

        DecoderPtr++;
        NoiseEnergyPtr++;
        SignalPtr++;
    } // printf("\n");

    if (BlockPhase == SyncBestBlockPhase) {
        SyncBestSignal     = BestSliceSignal;
        SyncBestFreqOffset = BestSliceOffset;
    } else {
        if (BestSliceSignal > SyncBestSignal) {
            SyncBestSignal     = BestSliceSignal;
            SyncBestBlockPhase = BlockPhase;
            SyncBestFreqOffset = BestSliceOffset;
        }
    }
    /*
       printf("MFSK_Synchronizer: %4.1f @ %3d:%3d\n",
       SyncBestSignal, SyncBestBlockPhase, SyncBestFreqOffset);
     */

    DecodeReference = (int) BlockPhase - (int) SyncBestBlockPhase;

    if (DecodeReference < 0) {
        DecodeReference += BlockPhases;
    }

    DecodeReference -= (int) (BlockPhases / 2);

    if (DecodeReference == 0) {
        float BestNoise = SyncNoiseEnergy[SyncBestBlockPhase][SyncBestFreqOffset].Output;

        if (BestNoise > 0) {
            BestNoise = sqrt(BestNoise);
        } else {
            BestNoise = 0;
        }
        const float MinNoise = (float) Parameters->SymbolsPerBlock / 10000;

        if (BestNoise < MinNoise) {
            BestNoise = MinNoise;
        }

        SyncSNR = SyncBestSignal / BestNoise;

        float                   NewPreciseFreqOffset;
        float                   SignalPeak;

        LowPass3_Filter<float> *Signal = SyncSignal[SyncBestBlockPhase];
        size_t                  FitIdx = Limit(SyncBestFreqOffset, (size_t) 1, (size_t) (FreqOffsets - 2));
        int FitOK = FitPeak(NewPreciseFreqOffset, SignalPeak, Signal[FitIdx - 1].Output, Signal[FitIdx].Output,
                            Signal[FitIdx + 1].Output);

        if (FitOK < 0) {
            NewPreciseFreqOffset = SyncBestFreqOffset;
        } else {
            NewPreciseFreqOffset = FitIdx + Limit(NewPreciseFreqOffset, (float) -1.0, (float) 1.0);
        }

        float  NewPreciseBlockPhase;
        size_t FitIdxL = SyncBestBlockPhase;

        SyncSignal.DecrPtr(FitIdxL);
        size_t FitIdxC = SyncBestBlockPhase;
        size_t FitIdxR = SyncBestBlockPhase;

        SyncSignal.IncrPtr(FitIdxR);
        FitOK = FitPeak(NewPreciseBlockPhase, SignalPeak, SyncSignal[FitIdxL][SyncBestFreqOffset].Output,
                        SyncSignal[FitIdxC][SyncBestFreqOffset].Output, SyncSignal[FitIdxR][SyncBestFreqOffset].Output);
        if (FitOK < 0) {
            NewPreciseBlockPhase = SyncBestBlockPhase;
        } else {
            NewPreciseBlockPhase += FitIdxC;
            SyncSignal.WrapPhase(NewPreciseBlockPhase);
        }

        float FreqDelta  = NewPreciseFreqOffset - PreciseFreqOffset;
        float PhaseDelta = NewPreciseBlockPhase - PreciseBlockPhase;

        SyncSignal.WrapDiffPhase(PhaseDelta);

        float DeltaDist2 = FreqDelta * FreqDelta + PhaseDelta * PhaseDelta;

        if ((DeltaDist2 <= 1.0) && (SyncSNR >= Parameters->RxSyncThreshold)) {
            StableLock = 1;
            FreqDrift.Process(FreqDelta, SyncFilterWeight);
            TimeDrift.Process(PhaseDelta / BlockPhases, SyncFilterWeight);
        } else {
            StableLock = 0;
            FreqDrift  = 0;
            TimeDrift  = 0;
        }

        PreciseFreqOffset = NewPreciseFreqOffset;
        PreciseBlockPhase = NewPreciseBlockPhase;
    }
    SyncSignal.IncrPtr(BlockPhase);
}

float MFSK_Synchronizer::FEC_SNR(void) {
    return SyncSNR;
}

float MFSK_Synchronizer::FrequencyOffset(void) {
    return (PreciseFreqOffset - (FreqOffsets / 2)) * Parameters->FFTbinBandwidth();
}

float MFSK_Synchronizer::FrequencyDriftRate(void) {
    return FreqDrift.Output * Parameters->FFTbinBandwidth() / Parameters->BlockPeriod();
}

float MFSK_Synchronizer::TimeDriftRate(void) {
    return TimeDrift.Output;
}
