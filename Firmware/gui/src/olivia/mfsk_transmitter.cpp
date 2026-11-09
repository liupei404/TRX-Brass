/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_transmitter.h"

MFSK_Transmitter::MFSK_Transmitter() {
    Init();
}

MFSK_Transmitter::~MFSK_Transmitter() {
    Free();
}

void MFSK_Transmitter::Init(void) {
    ModulatorOutput = 0;
    ConverterOutput = 0;
}

void MFSK_Transmitter::Free(void) {
    Input.Free();
    Monitor.Free();
    Encoder.Free();
    Modulator.Free();
    free(ModulatorOutput);
    ModulatorOutput = 0;
    free(ConverterOutput);
    ConverterOutput = 0;
}

int MFSK_Transmitter::Preset(MFSK_Parameters *NewParameters) {
    Parameters      = NewParameters;
    BitsPerSymbol   = Parameters->BitsPerSymbol;
    SymbolsPerBlock = Parameters->SymbolsPerBlock;

    // preset the input character buffer
    Input.Len = 1024;

    if (Input.Preset() < 0) {
        goto Error;
    }

    Monitor.Len = 256;

    if (Monitor.Preset() < 0) {
        goto Error;
    }

    // preset the encoder
    Encoder.BitsPerSymbol = BitsPerSymbol;

    if (Encoder.Preset() < 0) {
        goto Error;
    }

    // preset the modulator
    if (Modulator.Preset(Parameters) < 0) {
        goto Error;
    }

    if (ReallocArray(&ModulatorOutput, Modulator.OutputLen) < 0) {
        goto Error;
    }

    MaxOutputLen = (size_t) ceil(Parameters->SymbolSepar + 2);

    if (ReallocArray(&ConverterOutput, MaxOutputLen) < 0) {
        goto Error;
    }

    Reset();

    return 0;

Error:
    Free();
    return -1;
}

void MFSK_Transmitter::Reset(void) {
    Input.Reset();
    Monitor.Reset();
    SymbolPtr = 0;
    State     = 0;
}

void MFSK_Transmitter::Start(void) {
    State |= State_Running;
}

void MFSK_Transmitter::Stop(void) {
    State |= State_StopReq;
}

int MFSK_Transmitter::Running(void) {
    return State & State_Running;
}

int MFSK_Transmitter::PutChar(uint8_t Char) {
    return Input.Write(Char);
}

int MFSK_Transmitter::GetChar(uint8_t &Char) {
    return Monitor.Read(Char);
}

int MFSK_Transmitter::Output(float *&OutputPtr) {
    if (SymbolPtr == 0) // when at the block boundary
    {
        if ((State & State_StopReq) && Input.Empty()) // if the user requested to stop and no more characters
        {
            State = 0;
        }                               // then simply stop
        else if (State & State_Running) // otherwise when state is "running" then keep going
        {
            size_t Idx; // form a new block

            for (Idx = 0; Idx < BitsPerSymbol; Idx++) // loop over characters in a block
            {
                uint8_t Char;

                if (Input.Read(Char) <= 0) {
                    break; // get character from the input FIFO
                }
                InputBlock[Idx] = Char; // put it into the block
                Monitor.Write(Char);
            } // put this character into the monitor FIFO

            for (; Idx < BitsPerSymbol; Idx++) { // fill the unused part of the block
                InputBlock[Idx] = 0;
            }

            Encoder.EncodeBlock(InputBlock); // encode the new block
        }
    }

    if (State & State_Running) // if state is "running" then
    {
        Modulator.Send(Encoder.OutputBlock[SymbolPtr]); // send out the next symbol of encoded
                                                        // block through the modulator
        SymbolPtr += 1;

        if (SymbolPtr >= SymbolsPerBlock) {
            SymbolPtr = 0;
        }
    }

    int ModLen = Modulator.Output(ModulatorOutput); // get the modulator output

    for (size_t i = 0; i < ModLen; i++) {
        ConverterOutput[i] = ModulatorOutput[i]; // TODO Filter and window
    }

    OutputPtr = ConverterOutput;
    return ModLen;
}

int MFSK_Transmitter::Output(int16_t *Buffer) {
    float *OutputPtr;
    int    OutputLen = Output(OutputPtr);

    ConvertToS16(ConverterOutput, Buffer, OutputLen);
    return OutputLen;
}
