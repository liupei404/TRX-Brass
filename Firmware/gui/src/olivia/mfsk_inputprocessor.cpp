/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_inputprocessor.h"

MFSK_InputProcessor::MFSK_InputProcessor() {
    Init();
    Default();
}

MFSK_InputProcessor::~MFSK_InputProcessor() {
    Free();
}

void MFSK_InputProcessor::Init(void) {
    InpTap      = 0;
    OutTap      = 0;
    WindowShape = 0;
    FFT_Buff    = 0;
    Spectra[0]  = 0;
    Spectra[1]  = 0;
    Output      = 0;
    Energy      = 0;
}

void MFSK_InputProcessor::Free(void) {
    free(InpTap);
    InpTap = 0;
    free(OutTap);
    OutTap = 0;
    free(WindowShape);
    WindowShape = 0;
    free(FFT_Buff);
    FFT_Buff = 0;
    free(Spectra[0]);
    Spectra[0] = 0;
    free(Spectra[1]);
    Spectra[1] = 0;
    free(Output);
    Output = 0;
    free(Energy);
    Energy = 0;
    FFT.Free();
    Filter.Free();
}

void MFSK_InputProcessor::Default(void) {
    WindowLen    = 8192;
    LimiterLevel = 2.5;
}

int MFSK_InputProcessor::Preset(void) {
    size_t Idx;

    WrapMask         = WindowLen - 1;
    float ShapeScale = 2.0 / WindowLen;

    if (ReallocArray(&InpTap, WindowLen) < 0) {
        goto Error;
    }

    ClearArray(InpTap, WindowLen);
    InpTapPtr = 0;

    if (ReallocArray(&OutTap, WindowLen) < 0) {
        goto Error;
    }

    ClearArray(OutTap, WindowLen);
    OutTapPtr = 0;

    if (FFT.Preset(WindowLen) < 0) {
        goto Error;
    }

    if (ReallocArray(&FFT_Buff, WindowLen) < 0) {
        goto Error;
    }

    SliceSepar = WindowLen / 2;

    if (ReallocArray(&WindowShape, WindowLen) < 0) {
        goto Error;
    }

    for (Idx = 0; Idx < WindowLen; Idx++) {
        WindowShape[Idx] = ShapeScale * sqrt(1.0 - FFT.Twiddle[Idx].Re);
    }

    SpectraLen = WindowLen / 2;

    if (ReallocArray(&Spectra[0], SpectraLen) < 0) {
        goto Error;
    }

    if (ReallocArray(&Spectra[1], SpectraLen) < 0) {
        goto Error;
    }

    if (ReallocArray(&Output, WindowLen) < 0) {
        goto Error;
    }

    ClearArray(Output, WindowLen);

    if (ReallocArray(&Energy, SpectraLen) < 0) {
        goto Error;
    }

    Filter.Len = WindowLen / 16;

    if (Filter.Preset() < 0) {
        goto Error;
    }

    return 0;

Error:
    Free();
    return -1;
}

void MFSK_InputProcessor::Reset(void) {
    ClearArray(InpTap, WindowLen);
    InpTapPtr = 0;
    ClearArray(OutTap, WindowLen);
    OutTapPtr = 0;
}

void MFSK_InputProcessor::LimitSpectraPeaks(Cmpx<float> *Spectra, size_t BoxLen) {
    Filter.Len = BoxLen;
    Filter.Preset();

    size_t MaxFreq = 3 * (SpectraLen / 4);
    size_t Freq, Idx;

    for (Freq = 0; Freq < BoxLen; Freq++) {
        Filter.Process(Energy[Freq]);
    }

    float Threshold = LimiterLevel * LimiterLevel;

    for (Idx = BoxLen / 2; Freq < MaxFreq; Freq++, Idx++) {
        Filter.Process(Energy[Freq]);
        float Signal = Energy[Idx];
        float Limit  = (Filter.Output / BoxLen) * Threshold;

        if (Signal > Limit) {
            Spectra[Idx] *= sqrt(Limit / Signal);
            Energy[Idx] = Limit;
        }
    }
}

void MFSK_InputProcessor::LimitOutputPeaks(void) {
    size_t Idx;
    float  RMS = 0;

    for (Idx = 0; Idx < WindowLen; Idx++) {
        float Signal = Output[Idx];

        RMS += Signal * Signal;
    }

    RMS         = sqrt(RMS / WindowLen);
    float Limit = RMS * LimiterLevel;

    for (Idx = 0; Idx < WindowLen; Idx++) {
        float Signal = Output[Idx];

        if (Signal > Limit) {
            Output[Idx] = Limit;
        } else if (Signal < (-Limit)) {
            Output[Idx] = (-Limit);
        }
    }
}

void MFSK_InputProcessor::AverageEnergy(size_t Len) {
    Filter.Len = Len;
    Filter.Preset();

    size_t MaxFreq = 3 * (SpectraLen / 4);
    float  Scale   = 1.0 / Len;
    size_t Len2    = Len / 2;
    size_t Idx, Freq;

    for (Freq = 0; Freq < Len; Freq++) {
        Filter.Process(Energy[Freq]);
    }

    for (Idx = 0; Idx < Len2; Idx++) {
        Energy[Idx] = Filter.Output * Scale;
    }

    for (; Freq < MaxFreq; Freq++, Idx++) {
        Filter.Process(Energy[Freq]);
        Energy[Idx] = Filter.Output * Scale;
    }

    for (; Idx < SpectraLen; Idx++) {
        Energy[Idx] = Filter.Output * Scale;
    }
}

void MFSK_InputProcessor::ProcessSpectra(Cmpx<float> *Spectra) {
    size_t Freq;

    for (Freq = 0; Freq < SpectraLen; Freq++) {
        Energy[Freq] = Spectra[Freq].Energy();
    }

    LimitSpectraPeaks(Spectra, WindowLen / 64);
    LimitSpectraPeaks(Spectra, WindowLen / 64);
    LimitSpectraPeaks(Spectra, WindowLen / 64);

    AverageEnergy(WindowLen / 96);
    AverageEnergy(WindowLen / 64);

    for (Freq = 0; Freq < SpectraLen; Freq++) {
        float Corr = Energy[Freq];

        if (Corr <= 0) {
            continue;
        }

        Corr = 1.0 / sqrt(Corr);
        Spectra[Freq] *= Corr;
    }
}

void MFSK_InputProcessor::ProcessInpTap(float *Input) {
    for (size_t InpIdx = 0; InpIdx < SliceSepar; InpIdx++) {
        InpTap[InpTapPtr] = Input[InpIdx];
        InpTapPtr += 1;
        InpTapPtr &= WrapMask;
    }
}

void MFSK_InputProcessor::ProcessInpTap() {
    for (size_t InpIdx = 0; InpIdx < SliceSepar; InpIdx++) {
        InpTap[InpTapPtr] = 0;
        InpTapPtr += 1;
        InpTapPtr &= WrapMask;
    }
}

void MFSK_InputProcessor::ProcessInpWindow_Re(void) {
    for (size_t Time = 0; Time < WindowLen; Time++) {
        FFT_Buff[Time].Re = InpTap[InpTapPtr] * WindowShape[Time];
        InpTapPtr += 1;
        InpTapPtr &= WrapMask;
    }
}

void MFSK_InputProcessor::ProcessInpWindow_Im(void) {
    for (size_t Time = 0; Time < WindowLen; Time++) {
        FFT_Buff[Time].Im = InpTap[InpTapPtr] * WindowShape[Time];
        InpTapPtr += 1;
        InpTapPtr &= WrapMask;
    }
}

void MFSK_InputProcessor::ProcessOutWindow_Re(void) {
    for (size_t Time = 0; Time < WindowLen; Time++) {
        OutTap[OutTapPtr] += FFT_Buff[Time].Re * WindowShape[Time];
        OutTapPtr += 1;
        OutTapPtr &= WrapMask;
    }
}

void MFSK_InputProcessor::ProcessOutWindow_Im(void) {
    for (size_t Time = 0; Time < WindowLen; Time++) {
        OutTap[OutTapPtr] += FFT_Buff[Time].Im * WindowShape[Time];
        OutTapPtr += 1;
        OutTapPtr &= WrapMask;
    }
}

void MFSK_InputProcessor::ProcessOutTap(float *Output) {
    for (size_t OutIdx = 0; OutIdx < SliceSepar; OutIdx++) {
        Output[OutIdx]    = OutTap[OutTapPtr];
        OutTap[OutTapPtr] = 0;
        OutTapPtr += 1;
        OutTapPtr &= WrapMask;
    }
}

int MFSK_InputProcessor::Process(float *Input) {
    if (Input) {
        ProcessInpTap(Input);
    } else {
        ProcessInpTap();
    }

    ProcessInpWindow_Re();

    if (Input) {
        ProcessInpTap(Input + SliceSepar);
    } else {
        ProcessInpTap();
    }

    ProcessInpWindow_Im();

    FFT.Process(FFT_Buff);
    FFT.SeparTwoReals(FFT_Buff, Spectra[0], Spectra[1]);

    ProcessSpectra(Spectra[0]);
    ProcessSpectra(Spectra[1]);

    FFT.JoinTwoReals(Spectra[0], Spectra[1], FFT_Buff);
    FFT.Process(FFT_Buff);

    ProcessOutWindow_Re();
    ProcessOutTap(Output);
    ProcessOutWindow_Im();
    ProcessOutTap(Output + SliceSepar);

    LimitOutputPeaks();
    LimitOutputPeaks();

    return WindowLen;
}

int MFSK_InputProcessor::GetOutput(int16_t *Buffer) {
    const float   Scale = 32768.0;
    const int32_t Limit = 0x7FFF;
    size_t        Idx;

    for (Idx = 0; Idx < WindowLen; Idx++) {
        float Ampl = Output[Idx];

        Ampl *= Scale;
        int32_t Out = (int32_t) floor(Ampl + 0.5);

        if (Out > Limit) {
            Out = Limit;
        } else if (Out < (-Limit)) {
            Out = (-Limit);
        }

        Buffer[Idx] = (int16_t) Out;
    }

    return WindowLen;
}
