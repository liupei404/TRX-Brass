/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include "lvgl/src/drivers/display/fb/lv_linux_fbdev.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include "main.h"
#include "main_screen.h"
#include "styles.h"
#include "radio.h"
#include "swr.h"
#include "dsp.h"
#include "util.h"
#include "keyboard.h"
#include "keypad.h"
#include "audio.h"
#include "recorder.h"
#include "cw.h"
#include "cw_key.h"
#include "pannel.h"
#include "rtty.h"
#include "backlight.h"
#include "events.h"
#include "queue.h"
#include "gps.h"
#include "fpga/adc.h"
#include "fpga/dac.h"
#include "fpga/mb.h"
#include "python/python.h"
#include "mic.h"
#include "vt.h"
#include "hw/gpio.h"
#include "hw/iio.h"
#include "hw/power_meter.h"
#include "settings/bands.h"
#include "settings/modes.h"
#include "settings/options.h"
#include "settings/rf.h"
#include "settings/hw.h"
#include "olivia/olivia.h"
#include "bands/bands.h"
#include "dbus/dbus.h"
#include "power.h"

rotary_t                    *vol;
encoder_t                   *mfk;

static pthread_mutex_t      mux;
static volatile sig_atomic_t exit_requested;

static void request_exit(int signal_number) {
    (void) signal_number;
    exit_requested = 1;
}

static void lv_log_stderr_cb(lv_log_level_t level, const char *buf) {
    (void) level;
    fprintf(stderr, "%s\n", buf);
}

void brass_lv_lock() {
    pthread_mutex_lock(&mux);
}

void brass_lv_unlock() {
    pthread_mutex_unlock(&mux);
}

void main_exit() {
    iio_shutdown();
    settings_bands_save();
    settings_modes_save();
    settings_options_save();
    settings_rf_save();
    vt_enable();
    exit(1);
}

int main(void) {
    signal(SIGINT, request_exit);
    signal(SIGTERM, request_exit);
    vt_disable();
    power_boot_hold_check();        /* 长按开机的软件侧校验，默认只记录不误关机 */
    pthread_mutex_init(&mux, NULL);

    settings_modes_load();
    settings_bands_load();
    settings_hw_load();
    settings_options_load();
    settings_rf_load();

    lv_init();
    lv_libpng_init();

    lv_display_t *display = lv_linux_fbdev_create();
    if (display == NULL || lv_linux_fbdev_set_file(display, "/dev/fb0") != LV_RESULT_OK) {
        LV_LOG_ERROR("unable to initialize /dev/fb0");
        return 1;
    }

    mic_init();
    audio_init();
    recorder_init();
    queue_init();
    event_init();
    gpio_init();
    swr_init();
    iio_init();
    power_meter_init();
    bands_init();
    dbus_init();

    lv_timer_t *timer = lv_display_get_refr_timer(display);

    lv_timer_set_period(timer, 15);

    keyboard_init();

    vol = rotary_init("/dev/input/event0", 1);
    mfk = encoder_init("/dev/input/event1");
    rotary_t *main = rotary_init("/dev/input/event2", 8);
    keypad_t *keypad = keypad_init("/dev/input/event3");
    keypad_t *gpio = keypad_init("/dev/input/event4");

    if (vol == NULL || mfk == NULL || main == NULL || keypad == NULL || gpio == NULL) {
        return 1;
    }

    vol->left[VOL_EDIT] = KEY_VOL_LEFT_EDIT;
    vol->right[VOL_EDIT] = KEY_VOL_RIGHT_EDIT;

    vol->left[VOL_SELECT] = KEY_VOL_LEFT_SELECT;
    vol->right[VOL_SELECT] = KEY_VOL_RIGHT_SELECT;

    python_init();
    styles_init();

    lv_obj_t *main_obj = main_screen();
    if (main_obj == NULL) {
        return 1;
    }

    cw_init();
    cw_key_init();
    dsp_init();
    rtty_init();
    olivia_init();
    radio_init(main_obj);
    adc_init();
    dac_init();
    mb_init();

/*
    backlight_init();
    gps_init();
*/
    uint64_t prev_time = get_time();

    lv_scr_load(main_obj);
    bool first_loop = true;

    while (!exit_requested) {
        brass_lv_lock();

        uint64_t now = get_time();
        uint64_t delta = now - prev_time;

        lv_tick_inc(delta);
        prev_time = now;

        lv_timer_handler();

        queue_work();
        first_loop = false;

        brass_lv_unlock();
        usleep(100);
    }

    main_exit();
    return 0;
}
