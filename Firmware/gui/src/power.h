/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * 长按开关机统一阈值：开机与关机使用同一时长，避免按下时间不一致。
 * keypad.c 的 KEYPAD_LONG 判定与 LVGL indev 的 long_press_time 都取此值。
 */
#define POWER_LONG_PRESS_MS     1000

/*
 * 上电保持校验阈值。断电期间没有任何软件运行，真正的"长按开机"必须由
 * 电源自锁 / FPGA 计时电路保证；这里只提供 Linux 启动后的软件校验接口，
 * 见 power.c 的 power_boot_hold_check()。
 */
#define POWER_BOOT_HOLD_MS      1000

#ifndef POWER_BOOT_KEY_DEV
#define POWER_BOOT_KEY_DEV      "/dev/input/event3"
#endif

/* 电源键长按判定：纯逻辑，不依赖 LVGL 与硬件，可单独编译测试 */
typedef enum {
    POWER_KEY_IDLE = 0,
    POWER_KEY_PRESSED,
    POWER_KEY_LONG
} power_key_state_t;

typedef enum {
    POWER_KEY_EV_PRESS = 0,
    POWER_KEY_EV_RELEASE,
    POWER_KEY_EV_TICK
} power_key_ev_t;

typedef struct {
    power_key_state_t   state;
    uint32_t            held_ms;
    uint32_t            long_ms;
    uint32_t            tick_ms;
} power_key_fsm_t;

void power_key_fsm_init(power_key_fsm_t *fsm, uint32_t long_ms, uint32_t tick_ms);

/* 返回 true 表示刚好达到长按阈值，应执行开关机动作；短按与长按结束均返回 false */
bool power_key_fsm_event(power_key_fsm_t *fsm, power_key_ev_t ev);

/* 上电按键保持校验：纯逻辑 */
typedef enum {
    POWER_BOOT_PENDING = 0,
    POWER_BOOT_OK,
    POWER_BOOT_REJECT
} power_boot_result_t;

typedef struct {
    uint32_t            held_ms;
    uint32_t            required_ms;
    bool                key_down;
} power_boot_fsm_t;

void power_boot_fsm_init(power_boot_fsm_t *fsm, uint32_t required_ms, bool key_down);
power_boot_result_t power_boot_fsm_tick(power_boot_fsm_t *fsm, uint32_t elapsed_ms, bool key_down);

/* 长按关机：保存设置 -> sync -> 内核断电；内核不支持时有序退出 GUI */
void power_shutdown(void);

/* 上电后读取电源键状态，用于长按开机的软件侧校验 */
void power_boot_hold_check(void);
