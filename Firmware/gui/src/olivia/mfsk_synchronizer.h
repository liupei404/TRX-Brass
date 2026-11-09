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
#include "mfsk_softdecoder.h"

class MFSK_Synchronizer {
private:
    size_t                                 FreqOffsets; // number of possible frequency offsets
    size_t                                 BlockPhases; // number of possible time-phases within the FEC block
    MFSK_SoftDecoder                      *Decoder;     // array of decoders

    CircularBuffer<LowPass3_Filter<float>> SyncNoiseEnergy;  // FEC noise integrators
    CircularBuffer<LowPass3_Filter<float>> SyncSignal;       // FEC signal integrators
    float                                  SyncFilterWeight; // weight for the integrators

public:
    MFSK_Parameters       *Parameters;
    size_t                 BlockPhase;         // current running block time-phase
    float                  SyncBestSignal;     // best signal
    size_t                 SyncBestBlockPhase; // time-phase of the best signal        [FFT bins]
    size_t                 SyncBestFreqOffset; // frequency offset of the best signal  [FFT spectral slices]
    float                  SyncSNR;            // S/N corresponding to the SyncBestSignal
    int                    DecodeReference;    // when 0 then right in the middle of a FEC block
    float                  PreciseFreqOffset;  // precise interpolated frequency offset [FFT bins]
    float                  PreciseBlockPhase;  //                   and the FEC block phase [spectral slices]
    size_t                 StableLock;         // is 1 when the sync. looks stable
    LowPass3_Filter<float> FreqDrift;          // frequency drift rate [FFT bin / FEC block]
    LowPass3_Filter<float> TimeDrift;          // block phase (time) drift rate [ppm]

public:
    MFSK_Synchronizer();
    ~MFSK_Synchronizer();

    void  Init(void);
    void  Free(void);
    int   Preset(MFSK_Parameters *NewParameters);
    void  Reset(void);
    void  Process(float *Spectra);

    float FEC_SNR(void);
    float FrequencyOffset(void);
    float FrequencyDriftRate(void);
    float TimeDriftRate(void);
};
