/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "mfsk_softdemodulate.h"
#include "utils.h"
#include "gray.h"

void MFSK_SoftDemodulate(float *Symbol, float *SpectraEnergy, size_t BitsPerSymbol,
                         size_t CarrierSepar, int UseGrayCode, int SquareEnergy) {
    size_t Bit, Idx;

    for (Bit = 0; Bit < BitsPerSymbol; Bit++) {
        Symbol[Bit] = 0;
    }

    size_t     Carriers    = Exp2(BitsPerSymbol);
    float TotalEnergy = 0;
    size_t     Freq        = 0;

    for (Idx = 0; Idx < Carriers; Idx++) // loop over carriers
    {
        uint8_t SymbIdx = Idx;

        if (UseGrayCode) {
            SymbIdx = BinaryCode(SymbIdx);
        }

        float Energy = SpectraEnergy[Freq]; // energy for given carrier

        if (SquareEnergy) {
            Energy *= Energy; // square the energy (works better, but why ?)
        }

        TotalEnergy += Energy;
        uint8_t Mask = 1;

        for (Bit = 0; Bit < BitsPerSymbol; Bit++) // soft decision (contribution) for every bit
        {
            if (SymbIdx & Mask) {
                Symbol[Bit] -= Energy; // add or subtract the contribution
            } else {
                Symbol[Bit] += Energy; // depending on bit value
            }
            Mask <<= 1;
        }
        Freq += CarrierSepar;
    }

    if (TotalEnergy > 0) // normalize the soft decisions
    {
        for (Bit = 0; Bit < BitsPerSymbol; Bit++) {
            Symbol[Bit] /= TotalEnergy;
        }
    }
}
