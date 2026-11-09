/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <iio.h>

#include "iio_adc.h"

#define ADC_TRIGGER_HZ      2000

/* Small blocks preserve low latency; actual scan rate is measured on target. */

#define ADC_BLOCK_SCANS     2
#define ADC_TRIGGER_NAME    "brass-adc"
#define ADC_TRIGGER_PATH    "/sys/kernel/config/iio/triggers/hrtimer/" ADC_TRIGGER_NAME
#define ADC_RETRY_MS        2000

static struct iio_device *adc_dev;
static struct iio_channel *adc_fwd, *adc_rev;
static void (*publish_measurement)(int, int);
static pthread_t thread;
static bool thread_started;
static atomic_bool running;

/* Protects buffer lifetime against cancellation by the shutdown thread. */

static pthread_mutex_t      buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct iio_buffer    *active_buffer;

static int adc_error(const char *operation, int error) {
    fprintf(stderr, "ADC: %s: %s (%d)\n", operation, strerror(-error), error);

    return error;
}

static int write_control(const char *path, const char *value) {
    FILE *file = fopen(path, "w");

    if (!file)
        return adc_error(path, -errno);

    int ret = fputs(value, file) == EOF ? -(errno ? errno : EIO) : 0;

    if (fclose(file) && !ret)
        ret = -(errno ? errno : EIO);

    return ret ? adc_error(path, ret) : 0;
}

static int set_trigger(const char *name) {
    char path[PATH_MAX];

    snprintf(path, sizeof(path), "/sys/bus/iio/devices/%s/trigger/current_trigger", iio_device_get_id(adc_dev));

    return write_control(path, name);
}

static int configure_trigger(void) {
    if (mkdir(ADC_TRIGGER_PATH, 0755) && errno != EEXIST)
        return adc_error("create " ADC_TRIGGER_PATH " (check configfs/hrtimer support)", -errno);

    glob_t  paths = {0};
    int     found = -ENOENT;

    if (glob("/sys/bus/iio/devices/trigger*/name", 0, NULL, &paths)) {
        globfree(&paths);

        return adc_error("find " ADC_TRIGGER_NAME, found);
    }

    for (size_t i = 0; i < paths.gl_pathc; i++) {
        char    name[128] = {0}, frequency_path[PATH_MAX], rate[32];
        FILE    *file = fopen(paths.gl_pathv[i], "r");

        if (!file)
            continue;

        bool match = fgets(name, sizeof(name), file) != NULL;

        fclose(file);
        name[strcspn(name, "\r\n")] = '\0';

        if (!match || strcmp(name, ADC_TRIGGER_NAME))
            continue;

        snprintf(frequency_path, sizeof(frequency_path), "%.*ssampling_frequency",
                 (int) (strlen(paths.gl_pathv[i]) - strlen("name")), paths.gl_pathv[i]);

        snprintf(rate, sizeof(rate), "%d\n", ADC_TRIGGER_HZ);

        found = write_control(frequency_path, rate);
        break;
    }

    globfree(&paths);

    if (found)
        return adc_error("configure " ADC_TRIGGER_NAME, found);

    return set_trigger(ADC_TRIGGER_NAME "\n");
}

int iio_adc_set_sampling_frequency(struct iio_channel *channel) {
    int         ret = iio_channel_attr_write_longlong(channel, "sampling_frequency", IIO_ADC_RATE_SPS);
    long long   rate = 0;

    if (ret >= 0)
        ret = iio_channel_attr_read_longlong(channel, "sampling_frequency", &rate);

    if (ret < 0)
        return adc_error(iio_channel_get_id(channel), ret);

    if (rate != IIO_ADC_RATE_SPS || rate < ADC_TRIGGER_HZ)
        return adc_error("ADC sampling_frequency readback mismatch", -EINVAL);

    return 0;
}

static int configure_channel(struct iio_channel *channel) {
    const struct iio_data_format *format = iio_channel_get_data_format(channel);

    /* libiio handles sign extension, byte order and the advertised shift. */

    if (!iio_channel_is_scan_element(channel) || !format ||
        format->length != 16 || !format->is_signed || format->repeat != 1)
        return adc_error("unsupported ADC scan format (expected one signed 16-bit word)", -EINVAL);

    int ret = iio_adc_set_sampling_frequency(channel);
    if (ret < 0)
        return ret;

    iio_channel_enable(channel);
    return 0;
}

static void destroy_buffer(struct iio_buffer *buffer) {
    pthread_mutex_lock(&buffer_mutex);
    active_buffer = NULL;

    if (buffer)
        iio_buffer_destroy(buffer);

    pthread_mutex_unlock(&buffer_mutex);
    iio_channel_disable(adc_fwd);
    iio_channel_disable(adc_rev);
}

static int acquire(struct iio_buffer *buffer) {
    uint64_t last_log = 0, scans_total = 0, blocks = 0;

    while (atomic_load(&running)) {
        ssize_t     bytes = iio_buffer_refill(buffer);

        if (!atomic_load(&running))
            return 0;

        if (bytes < 0)
            return adc_error("buffer refill", (int) bytes);

        ptrdiff_t step = iio_buffer_step(buffer);

        if (step <= 0 || bytes == 0 || bytes % step)
            return adc_error("invalid refill scan length", -EIO);

        char    *fwd = iio_buffer_first(buffer, adc_fwd);
        char    *rev = iio_buffer_first(buffer, adc_rev);
        char    *end = iio_buffer_end(buffer);
        int     peak_fwd = INT_MIN, peak_rev = INT_MIN;

        size_t scans = (size_t) (bytes / step);

        for (size_t n = 0; n < scans; n++) {
            if (end - fwd < (ptrdiff_t) sizeof(int16_t) ||
                end - rev < (ptrdiff_t) sizeof(int16_t)) 
                return adc_error("incomplete ADC scan", -EIO);

            int16_t sample_fwd, sample_rev;

            iio_channel_convert(adc_fwd, &sample_fwd, fwd);
            iio_channel_convert(adc_rev, &sample_rev, rev);

            if (sample_fwd > peak_fwd) peak_fwd = sample_fwd;
            if (sample_rev > peak_rev) peak_rev = sample_rev;

            if (n + 1 < scans) {
                fwd += step;
                rev += step;
            }
        }

        /* Preserve the existing SWR calibration/filtering in the consumer. */

        publish_measurement(peak_fwd, peak_rev);

        scans_total += scans;
        blocks++;
    }

    return 0;
}

static void *adc_thread(void *unused) {
    (void) unused;

    while (atomic_load(&running)) {
        struct iio_buffer *buffer = NULL;
        bool trigger_attached = false;

        /* This device's buffer contains only forward/reflected power. */

        for (unsigned int i = 0; i < iio_device_get_channels_count(adc_dev); i++)
            iio_channel_disable(iio_device_get_channel(adc_dev, i));

        if (configure_channel(adc_fwd) || configure_channel(adc_rev))
            goto retry;

        if (configure_trigger())
            goto retry;

        trigger_attached = true;

        int ret = iio_device_set_kernel_buffers_count(adc_dev, 1);

        if (ret < 0) {
            adc_error("set kernel buffer count", ret);
            goto retry;
        }

        buffer = iio_device_create_buffer(adc_dev, ADC_BLOCK_SCANS, false);

        if (!buffer) {
            adc_error("create two-channel buffer (check patched ADS1015 driver)", -errno);
            goto retry;
        }

        pthread_mutex_lock(&buffer_mutex);
        active_buffer = buffer;
        pthread_mutex_unlock(&buffer_mutex);

        acquire(buffer);
retry:
        destroy_buffer(buffer);

        if (trigger_attached)
            set_trigger("\n");

        /* Bounded retry delay, also allowing prompt shutdown after init errors. */

        for (int ms = 0; ms < ADC_RETRY_MS && atomic_load(&running); ms += 20)
            usleep(20000);
    }
    return NULL;
}

int iio_adc_start(struct iio_device *dev, struct iio_channel *fwd,
                  struct iio_channel *rev, void (*publish)(int, int)) {
    if (thread_started)
        return -EBUSY;

    if (!dev || !fwd || !rev || !publish)
        return -EINVAL;

    adc_dev = dev;
    adc_fwd = fwd;
    adc_rev = rev;

    publish_measurement = publish;
    atomic_store(&running, true);

    int ret = pthread_create(&thread, NULL, adc_thread, NULL);

    if (ret) {
        atomic_store(&running, false);
        return adc_error("create worker", -ret);
    }

    thread_started = true;
    return 0;
}

void iio_adc_stop(void) {
    if (!thread_started)
        return;

    atomic_store(&running, false);
    pthread_mutex_lock(&buffer_mutex);

    if (active_buffer)
        iio_buffer_cancel(active_buffer);

    pthread_mutex_unlock(&buffer_mutex);
    pthread_join(thread, NULL);
    thread_started = false;
}
