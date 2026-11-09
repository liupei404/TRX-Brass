/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_receiver.h"

MFSK_Receiver::MFSK_Receiver() {
    Init();
}

MFSK_Receiver::~MFSK_Receiver() {
    Free();
}

void MFSK_Receiver::Init(void) {
}

void MFSK_Receiver::Free(void) {
    InputProcessor.Free();
    Demodulator.Free();
    Synchronizer.Free();
    Decoder.Free();
    Output.Free();
}

int MFSK_Receiver::Preset(MFSK_Parameters *NewParameters) {
    Parameters               = NewParameters;

    InputProcessor.WindowLen = 32 * Parameters->SymbolSepar;

    if (InputProcessor.Preset() < 0) {
        goto Error;
    }

    if (Demodulator.Preset(Parameters) < 0) {
        goto Error;
    }

    if (Synchronizer.Preset(Parameters) < 0) {
        goto Error;
    }

    if (Decoder.Preset(Parameters) < 0) {
        goto Error;
    }

    Output.Len = 1024;
    if (Output.Preset() < 0) {
        goto Error;
    }

    return 0;

Error:
    Free();
    return -1;
}

void MFSK_Receiver::Reset(void) {
    InputProcessor.Reset();
    Demodulator.Reset();
    Synchronizer.Reset();
    Output.Reset();
}

float MFSK_Receiver::SyncSNR(void) {
    return Synchronizer.FEC_SNR();
}

float MFSK_Receiver::FrequencyOffset(void) {
    return Synchronizer.FrequencyOffset();
}

float MFSK_Receiver::FrequencyDrift(void) {
    return Synchronizer.FrequencyDriftRate();
}

float MFSK_Receiver::TimeDrift(void) {
    return Synchronizer.TimeDriftRate();
}

float MFSK_Receiver::InputSNRdB(void) {
    return Decoder.InputSNRdB();
}

int MFSK_Receiver::WindowLen() {
    return InputProcessor.WindowLen;
}

int MFSK_Receiver::GetChar(uint8_t &Char) {
    return Output.Read(Char);
}

void MFSK_Receiver::ProcessInputBuffer(float *Input) {
    InputProcessor.Process(Input);

    for (size_t Idx = 0; Idx < InputProcessor.WindowLen; Idx += Parameters->SymbolSepar) {
        ProcessSymbol(InputProcessor.Output + Idx);
    }
}

void MFSK_Receiver::ProcessSymbol(float *Input) {
    size_t SpectraPerSymbol = Parameters->SpectraPerSymbol;

    Demodulator.Process(Input);
    int HistOfs;

    for (HistOfs = (-SpectraPerSymbol); HistOfs < 0; HistOfs++) {
        float *Spectra = Demodulator.HistoryPtr(HistOfs);

        Synchronizer.Process(Spectra);
        size_t SpectraPerBlock = Parameters->SpectraPerBlock;

        if (Synchronizer.DecodeReference == 0) {
            if (Synchronizer.StableLock) {
                int   TimeOffset = (HistOfs - ((Parameters->RxSyncIntegLen + 1) * SpectraPerBlock +
                                             SpectraPerBlock / 2 - 1));

                int   FreqOffset = Synchronizer.SyncBestFreqOffset;
                float BestSignal = 0;
                int   BestTime   = 0;
                int   BestFreq   = 0;
                int   FreqSearch;

                for (FreqSearch = (-1); FreqSearch <= 1; FreqSearch++) {
                    int TimeSearch;

                    for (TimeSearch = (-2); TimeSearch <= 2; TimeSearch++) {
                        int Error = Demodulator.PickBlock(Decoder.Input, TimeOffset + TimeSearch,
                                                          FreqOffset + FreqSearch);
                        if (Error < 0) {
                            continue;
                        }

                        Decoder.Process(8);

                        float Signal = Decoder.Input_SignalEnergy;

                        if (Signal > BestSignal) {
                            BestSignal = Signal;
                            BestFreq   = FreqSearch;
                            BestTime   = TimeSearch;
                        }
                    }
                }

                Demodulator.PickBlock(Decoder.Input, TimeOffset + BestTime, FreqOffset + BestFreq);
                Decoder.Process(32);
                Decoder.WriteOutputBlock(Output);
            }
        }
    }
}
