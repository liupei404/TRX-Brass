/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdbool.h>
#include "lvgl/lvgl.h"
#include "xvrt.h"
#include "gpio.h"
#include "src/settings/hw.h"

#define REG0_FRACT(x)                   (((x) & 0xFFF) << 3)
#define REG0_INT(x)                     (((x) & 0xFFFF) << 15)

#define REG1_MOD(x)                     (((x) & 0xFFF) << 3)
#define REG1_PHASE(x)                   (((x) & 0xFFF) << 15)
#define REG1_PRESCALER                  (1 << 27)

#define REG2_COUNTER_RESET_EN           (1 << 3)
#define REG2_CP_THREESTATE_EN           (1 << 4)
#define REG2_POWER_DOWN_EN              (1 << 5)
#define REG2_PD_POLARITY_POS            (1 << 6)
#define REG2_LDP_6ns                    (1 << 7)
#define REG2_LDP_10ns                   (0 << 7)
#define REG2_LDF_FRACT_N                (0 << 8)
#define REG2_LDF_INT_N                  (1 << 8)
#define REG2_CHARGE_PUMP_CURR_uA(x)     (((((x)-312) / 312) & 0xF) << 9)
#define REG2_DOUBLE_BUFF_EN             (1 << 13)
#define REG2_10BIT_R_CNT(x)             ((x) << 14)
#define REG2_RDIV2_EN                   (1 << 24)
#define REG2_RMULT2_EN                  (1 << 25)
#define REG2_MUXOUT(x)                  ((x) << 26)
#define REG2_NOISE_MODE(x)              (((unsigned)(x)) << 29)
#define MUXOUT_THREESTATE               0
#define MUXOUT_DVDD                     1
#define MUXOUT_GND                      2
#define MUXOUT_R_DIV_OUT                3
#define MUXOUT_N_DIV_OUT                4
#define MUXOUT_ANALOG_LOCK_DETECT       5
#define MUXOUT_DIGITAL_LOCK_DETECT      6

#define REG3_12BIT_CLKDIV(x)            ((x) << 3)
#define REG3_12BIT_CLKDIV_MODE(x)       ((x) << 16)
#define REG3_12BIT_CSR_EN               (1 << 18)
#define REG3_CHARGE_CANCELLATION_EN     (1 << 21)
#define REG3_ANTI_BACKLASH_3ns_EN       (1 << 22)
#define REG3_BAND_SEL_CLOCK_MODE_HIGH   (1 << 23)

#define REG4_OUTPUT_PWR(x)              ((x) << 3)
#define REG4_RF_OUT_EN                  (1 << 5)
#define REG4_AUX_OUTPUT_PWR(x)          ((x) << 6)
#define REG4_AUX_OUTPUT_EN              (1 << 8)
#define REG4_AUX_OUTPUT_FUND            (1 << 9)
#define REG4_AUX_OUTPUT_DIV             (0 << 9)
#define REG4_MUTE_TILL_LOCK_EN          (1 << 10)
#define REG4_VCO_PWRDOWN_EN             (1 << 11)
#define REG4_8BIT_BAND_SEL_CLKDIV(x)    ((x) << 12)
#define REG4_RF_DIV_SEL(x)              ((x) << 20)
#define REG4_FEEDBACK_DIVIDED           (0 << 23)
#define REG4_FEEDBACK_FUND              (1 << 23)

#define REG5_LD_PIN_MODE_LOW            (0 << 22)
#define REG5_LD_PIN_MODE_DIGITAL        (1 << 22)
#define REG5_LD_PIN_MODE_HIGH           (3 << 22)
#define REG5_RESERVED                   (3 << 19)

#define REG2_VAL                        (REG2_MUXOUT(MUXOUT_DIGITAL_LOCK_DETECT) | REG2_CHARGE_PUMP_CURR_uA(2500) | REG2_PD_POLARITY_POS | 2)

static uint32_t prev_freq = 0;
static bool     first = true;

static uint32_t regs[6] = {
    0x0,
    (REG1_PHASE(1) | REG1_MOD(2) | 1),
    0x0,
    (REG3_12BIT_CLKDIV(150) | 3),
    0x0,
    (REG5_LD_PIN_MODE_DIGITAL | REG5_RESERVED | 5)
};

static void send_regs() {
    if (first) {
        for (int i = 5; i >= 0; i--) {
            gpio_send_adf(regs[i]);
        }
        first = false;
    } else {
        gpio_send_adf(regs[4]);
        gpio_send_adf(regs[0]);
    }
}

bool xvrt_update(const uint32_t freq) {
    if (freq != prev_freq) {
        uint32_t int_val;

        prev_freq = freq;

        if (freq > 110000000 && freq < 120000000) {
            int_val = freq / 125000 * 32;

            regs[4] = REG4_FEEDBACK_FUND | REG4_RF_DIV_SEL(6) | REG4_8BIT_BAND_SEL_CLKDIV(1) | REG4_RF_OUT_EN | REG4_OUTPUT_PWR(3) | 4;
            regs[0] = REG0_INT(int_val) | REG0_FRACT(0) | 0;

            send_regs();
        } else if (freq > 4000000 && freq < 415000000) {
            int_val = freq / 125000 * 8;

            regs[4] = REG4_FEEDBACK_FUND | REG4_RF_DIV_SEL(3) | REG4_8BIT_BAND_SEL_CLKDIV(1) | REG4_RF_OUT_EN | REG4_OUTPUT_PWR(3) | 4;
            regs[0] = REG0_INT(int_val) | REG0_FRACT(0) | 0;

            send_regs();
        } else {
            LV_LOG_INFO("Wrong shift %i", freq);
            return false;
        }

        LV_LOG_INFO("New shift %i", freq);
        return true;
    }

    return false;
}

void xvrt_load_tcxo_freq() {
    if (hw->tcxo_freq < 10000000 || hw->tcxo_freq > 13000000) {
        LV_LOG_ERROR("Invalid TCXO freq %i", hw->tcxo_freq);
        return;
    }

    uint16_t cnt = hw->tcxo_freq / 125000;

    regs[2] = REG2_VAL | REG2_10BIT_R_CNT(cnt);
    first = true;
}
