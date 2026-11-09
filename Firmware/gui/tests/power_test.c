/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI - power module unit test (host build)
 *
 *  Build:
 *    gcc -std=c11 -Wall -Wextra -Werror -DPOWER_TEST_STUB -Isrc \
 *        tests/power_test.c src/power.c -o power_test
 */

#include <assert.h>
#include <stdio.h>

#include "power.h"

/* power.c 的外部依赖在本测试中打桩，避免引入 LVGL / settings / 真正断电 */
static int save_calls;
static int shutdown_calls;

void iio_shutdown(void) { shutdown_calls++; }
void settings_bands_save(void) { save_calls++; }
void settings_modes_save(void) { save_calls++; }
void settings_options_save(void) { save_calls++; }
void settings_rf_save(void) { save_calls++; }
void vt_enable(void) { }
void main_exit(void) { assert(0 && "main_exit must not be called in test"); }

static void test_short_press_does_not_poweroff(void) {
    power_key_fsm_t fsm;

    power_key_fsm_init(&fsm, POWER_LONG_PRESS_MS, 100);
    assert(fsm.state == POWER_KEY_IDLE);
    assert(!power_key_fsm_event(&fsm, POWER_KEY_EV_PRESS));

    for (int i = 0; i < 9; i++) {
        assert(!power_key_fsm_event(&fsm, POWER_KEY_EV_TICK));
    }

    assert(!power_key_fsm_event(&fsm, POWER_KEY_EV_RELEASE));
    assert(fsm.state == POWER_KEY_IDLE);
    assert(fsm.held_ms == 0);
}

static void test_long_press_fires_once(void) {
    power_key_fsm_t fsm;

    power_key_fsm_init(&fsm, POWER_LONG_PRESS_MS, 100);
    assert(!power_key_fsm_event(&fsm, POWER_KEY_EV_PRESS));

    for (int i = 0; i < 9; i++) {
        assert(!power_key_fsm_event(&fsm, POWER_KEY_EV_TICK));
    }

    assert(power_key_fsm_event(&fsm, POWER_KEY_EV_TICK));
    assert(fsm.state == POWER_KEY_LONG);

    /* 不能重复触发，长按松开也不再次触发 */
    assert(!power_key_fsm_event(&fsm, POWER_KEY_EV_TICK));
    assert(!power_key_fsm_event(&fsm, POWER_KEY_EV_RELEASE));
    assert(fsm.state == POWER_KEY_IDLE);
}

static void test_boot_hold(void) {
    power_boot_fsm_t boot;

    power_boot_fsm_init(&boot, POWER_BOOT_HOLD_MS, true);
    assert(power_boot_fsm_tick(&boot, 400, true) == POWER_BOOT_PENDING);
    assert(power_boot_fsm_tick(&boot, 600, true) == POWER_BOOT_OK);
    assert(power_boot_fsm_tick(&boot, 0, false) == POWER_BOOT_OK);

    power_boot_fsm_init(&boot, POWER_BOOT_HOLD_MS, false);
    assert(power_boot_fsm_tick(&boot, 0, false) == POWER_BOOT_REJECT);
}

static void test_shutdown_saves_settings(void) {
    save_calls = 0;
    shutdown_calls = 0;

    power_shutdown();       /* POWER_TEST_STUB 下不会真的断电 */

    assert(save_calls == 4);
    assert(shutdown_calls == 1);
}

int main(void) {
    test_short_press_does_not_poweroff();
    test_long_press_fires_once();
    test_boot_hold();
    test_shutdown_saves_settings();

    printf("power_test: 4/4 passed (long press %u ms, boot hold %u ms)\n",
           POWER_LONG_PRESS_MS, POWER_BOOT_HOLD_MS);
    return 0;
}
