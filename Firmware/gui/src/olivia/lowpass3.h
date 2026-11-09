/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Based on (c) 2006, Pawel Jalocha
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

template <class Type> class LowPass3_Filter {
public:
    Type                                            Out1, Out2, Output;

    template <class InpType, class WeightType> void Process(InpType Inp, WeightType Weight, WeightType Feedback = 0.1) {
        Weight *= 2.0;
        Type DiffI1 = Inp;
        DiffI1 -= Out1;
        Type Diff12 = Out1;
        Diff12 -= Out2;
        Type Diff23 = Out2;
        Diff23 -= Output;
        DiffI1 *= Weight;
        Out1 += DiffI1;
        Diff12 *= Weight;
        Out2 += Diff12;
        Diff23 *= Weight;
        Output += Diff23;
        Diff23 *= Feedback;
        Out2 += Diff23;
    }

    template <class LevelType> void operator=(LevelType Level) {
        Out1   = Level;
        Out2   = Level;
        Output = Level;
    }

    template <class LevelType> void Set(LevelType Level = 0) {
        Out1   = Level;
        Out2   = Level;
        Output = Level;
    }
};
