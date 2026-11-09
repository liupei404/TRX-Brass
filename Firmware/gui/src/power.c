/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE         /* sync() 等 POSIX 声明在 -std=c11 下需要 */
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>

#include "power.h"

#ifndef POWER_TEST_STUB
#include <sys/reboot.h>
#endif

/*
 * 这些函数由 GUI 其它模块提供。此处只做外部声明而不 include 它们的头文件，
 * 使本模块可以脱离 LVGL / settings 依赖单独编译测试（POWER_TEST_STUB）。
 */
extern void iio_shutdown(void);
extern void settings_bands_save(void);
extern void settings_modes_save(void);
extern void settings_options_save(void);
extern void settings_rf_save(void);
extern void vt_enable(void);
extern void main_exit(void);

void power_key_fsm_init(power_key_fsm_t *fsm, uint32_t long_ms, uint32_t tick_ms) {
    if (fsm == NULL) {
        return;
    }

    fsm->state = POWER_KEY_IDLE;
    fsm->held_ms = 0;
    fsm->long_ms = long_ms ? long_ms : POWER_LONG_PRESS_MS;
    fsm->tick_ms = tick_ms ? tick_ms : 1;
}

bool power_key_fsm_event(power_key_fsm_t *fsm, power_key_ev_t ev) {
    if (fsm == NULL) {
        return false;
    }

    switch (ev) {
        case POWER_KEY_EV_PRESS:
            fsm->state = POWER_KEY_PRESSED;
            fsm->held_ms = 0;
            return false;

        case POWER_KEY_EV_TICK:
            if (fsm->state != POWER_KEY_PRESSED) {
                return false;
            }

            fsm->held_ms += fsm->tick_ms;

            if (fsm->held_ms >= fsm->long_ms) {
                fsm->state = POWER_KEY_LONG;
                return true;
            }

            return false;

        case POWER_KEY_EV_RELEASE:
        default:
            /* 短按（未达阈值）与长按松开都不触发关机；长按动作已在 TICK 发出一次 */
            fsm->state = POWER_KEY_IDLE;
            fsm->held_ms = 0;
            return false;
    }
}

void power_boot_fsm_init(power_boot_fsm_t *fsm, uint32_t required_ms, bool key_down) {
    if (fsm == NULL) {
        return;
    }

    fsm->held_ms = 0;
    fsm->required_ms = required_ms ? required_ms : POWER_BOOT_HOLD_MS;
    fsm->key_down = key_down;
}

power_boot_result_t power_boot_fsm_tick(power_boot_fsm_t *fsm, uint32_t elapsed_ms, bool key_down) {
    if (fsm == NULL) {
        return POWER_BOOT_PENDING;
    }

    fsm->key_down = key_down;

    if (key_down) {
        fsm->held_ms += elapsed_ms;

        return fsm->held_ms >= fsm->required_ms ? POWER_BOOT_OK : POWER_BOOT_PENDING;
    }

    /* 松开时按累计保持时长判定：达到阈值即认为长按开机成立 */
    return fsm->held_ms >= fsm->required_ms ? POWER_BOOT_OK : POWER_BOOT_REJECT;
}

void power_shutdown(void) {
    fprintf(stderr, "TRX Brass: long press -> power off\n");

    /* 与 main_exit() 相同的落盘顺序，保证关机不丢设置 */
    iio_shutdown();
    settings_bands_save();
    settings_modes_save();
    settings_options_save();
    settings_rf_save();
    vt_enable();
    sync();

#ifndef POWER_TEST_STUB
    if (reboot(RB_POWER_OFF) != 0) {
        perror("TRX Brass: reboot(RB_POWER_OFF) failed");

        /*
         * 内核没有可用断电驱动时，至少让 GUI 有序退出。
         * main_exit() 的 exit(1) 会被 brass_daemon 识别为有意退出，不再无限重启。
         */
        main_exit();
    }
#endif
}

void power_boot_hold_check(void) {
    int fd = open(POWER_BOOT_KEY_DEV, O_RDONLY | O_NONBLOCK);

    if (fd < 0) {
        /* 读不到按键状态时保持开机，绝不因为校验失败而误关机 */
        fprintf(stderr, "TRX Brass: power key state unavailable, keep powered on\n");
        return;
    }

    uint8_t key_bits[(KEY_MAX + 7) / 8] = { 0 };
    bool    key_down = false;

    if (ioctl(fd, EVIOCGKEY(sizeof(key_bits)), key_bits) == 0) {
        key_down = (key_bits[KEY_POWER / 8] & (1u << (KEY_POWER % 8))) != 0;
    } else {
        fprintf(stderr, "TRX Brass: EVIOCGKEY failed, keep powered on\n");
    }

    close(fd);
    fprintf(stderr, "TRX Brass: power key at boot: %s (long press %u ms)\n",
            key_down ? "held" : "released", POWER_LONG_PRESS_MS);

#ifdef POWER_BOOT_HOLD_STRICT
    /*
     * 严格模式：未检测到电源键保持按下就断电，用软件模拟"长按开机"。
     * 只有在 U-Boot / FPGA 能把上电瞬间的按键保持状态传递进来时才可启用，
     * 否则 GUI 启动延迟会造成误关机，故默认关闭。
     */
    if (!key_down) {
        fprintf(stderr, "TRX Brass: boot hold not satisfied, powering off\n");
        power_shutdown();
    }
#endif
}
