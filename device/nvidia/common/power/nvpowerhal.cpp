/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "powerHAL::common"

#include "powerhal.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/input.h>

#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "timeoutpoker.h"

#define POWER_CAP_PROP "persist.sys.NV_PBC_PWR_LIMIT"
#define LED_BREATHE_PROP "persist.sys.NV_LED_BREATHE"

#define EDP_MODE_NODE "/sys/kernel/tegra_core_edp/profile"
#define HOST1X_FLOOR_NODE "/sys/kernel/tegra_floor/h1x_floor_state"
#define EMC_FLOOR_NODE "/sys/kernel/tegra_floor/emc_floor_state"
#define CBUS_FLOOR_NODE "/sys/kernel/tegra_floor/cbus_floor_state"
#define SDHCI_BOOST_NODE "/sys/devices/platform/sdhci-tegra.0/handle_turbo_mode"

#define PMU_MODE_NODE "/sys/bus/platform/devices/palmas-pmic/auto_smps45_ctrl"
#define DEFAULT_PMU_MODE 0
#define THREE_PHASE_PMU_MODE 1

#define AP_FAN_STATE_CAP_DEFAULT 3
#define AP_VOLT_TEMP_MODE_DEFAULT 0
#define uC_DEFAULT_INPUT_NUMBER 1

#define uC_INPUT_NAME "NVIDIA Corporation NVIDIA Controller v01.01\n"

static int uC_input_number = uC_DEFAULT_INPUT_NUMBER;

static bool get_property_bool(const char *key, bool default_value);

static int old_edp_state = -1;

static void set_led_state(int on)
{
    struct input_event ev_out;
    int len;
    char buf[80];
    int size;
    int fd;
    bool breathe = get_property_bool(LED_BREATHE_PROP, true);
    char path[20];

    size = sizeof(struct input_event);
    on = (on == 0)?0:1;
    sprintf(path, "/dev/input/event%d", uC_input_number);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    ev_out.type = EV_LED;
    if (on == 1) {
        ev_out.code = LED_CAPSL;
    } else {
        if (breathe == true)
            ev_out.code = LED_SCROLLL;
	else
            ev_out.code = LED_COMPOSE;
    }
    ev_out.value = 1;

    len = write(fd, &ev_out, size);

    usleep(10000);

    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    ev_out.value = 0;

    len = write(fd, &ev_out, size);

    usleep(10000);

    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

void sysfs_write(const char *path, const char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }
    close(fd);
}

void sysfs_read(const char *path, char *s, int size)
{
    int len;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, s, size);
        ALOGE("Error opening %s: %s\n", path, s);
        return;
    }

    len = read(fd, s, size);
    close(fd);

    if (len < 0) {
        strerror_r(errno, s, size);
        ALOGE("Error reading from %s: %s\n", path, s);
    }
}

bool sysfs_exists(const char *path)
{
    bool val;
    int fd = open(path, O_RDONLY);

    val = fd < 0 ? false : true;
    close(fd);

    return val;
}

bool is_available_frequency(struct powerhal_info *pInfo, int freq)
{
    int i;

    for(i = 0; i < pInfo->num_available_frequencies; i++) {
        if(pInfo->available_frequencies[i] == freq)
            return true;
    }

    return false;
}

static int get_input_count(void)
{
    int i = 0;
    int ret;
    char path[80];
    char name[50];

    while(1)
    {
        sprintf(path, "/sys/class/input/input%d/name", i);
        ret = access(path, F_OK);
        if (ret < 0)
            break;
        memset(name, 0, 50);
        sysfs_read(path, name, 50);
        if (0 == strcmp(name, uC_INPUT_NAME))
            uC_input_number = i;
        ALOGI("input device id:%d present with name:%s\n", i++, name);
    }
    return i;
}

static void find_input_device_ids(struct powerhal_info *pInfo)
{
    int i = 0;
    int status;
    int count = 0;
    char path[80];
    char name[MAX_CHARS];

    while (1)
    {
        sprintf(path, "/sys/class/input/input%d/name", i);
        if (access(path, F_OK) < 0)
            break;
        else {
            memset(name, 0, MAX_CHARS);
            sysfs_read(path, name, MAX_CHARS);
            for (int j = 0; j < pInfo->input_cnt; j++) {
                status = (-1 == pInfo->input_devs[j].dev_id)
                    && (0 == strncmp(name,
                    pInfo->input_devs[j].dev_name, MAX_CHARS));
                if (status) {
                    ++count;
                    pInfo->input_devs[j].dev_id = i;
                    ALOGI("find_input_device_ids: %d %s",
                        pInfo->input_devs[j].dev_id,
                        pInfo->input_devs[j].dev_name);
                }
            }
            ++i;
        }

        if (count == pInfo->input_cnt)
            break;
    }
}

static int check_hint(struct powerhal_info *pInfo, power_hint_t hint, uint64_t *t)
{
    struct timespec ts;
    uint64_t time;

    if (hint >= POWER_HINT_COUNT)
        return -1;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    time = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

    if (pInfo->hint_time[hint] && pInfo->hint_interval[hint] &&
        (time - pInfo->hint_time[hint] < pInfo->hint_interval[hint]))
        return -1;

    *t = time;

    return 0;
}

void common_power_open(struct powerhal_info *pInfo)
{
    int i;
    int size = 128;
    char *pch;

    if (0 == pInfo->input_devs || 0 == pInfo->input_cnt)
        pInfo->input_cnt = get_input_count();
    else
        find_input_device_ids(pInfo);

    // Initialize timeout poker
    Barrier readyToRun;
    pInfo->mTimeoutPoker = new TimeoutPoker(&readyToRun);
    readyToRun.wait();

    // Read available frequencies
    char *buf = (char*)malloc(sizeof(char) * size);
    sysfs_read("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",
               buf, size);

    // Determine number of available frequencies
    pch = strtok(buf, " ");
    pInfo->num_available_frequencies = -1;
    while(pch != NULL)
    {
        pch = strtok(NULL, " ");
        pInfo->num_available_frequencies++;
    }

    // Store available frequencies in a lookup array
    pInfo->available_frequencies = (int*)malloc(sizeof(int) * pInfo->num_available_frequencies);
    sysfs_read("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",
               buf, size);
    pch = strtok(buf, " ");
    for(i = 0; i < pInfo->num_available_frequencies; i++)
    {
        pInfo->available_frequencies[i] = atoi(pch);
        pch = strtok(NULL, " ");
    }

    // Store LP cluster max frequency
    sysfs_read("/sys/devices/system/cpu/cpuquiet/tegra_cpuquiet/idle_top_freq",
                buf, size);
    pInfo->lp_max_frequency = atoi(buf);

    // Initialize hint intervals in usec
    pInfo->hint_interval[POWER_HINT_INTERACTION] = 1000000;
    pInfo->hint_interval[POWER_HINT_APP_PROFILE] = 1000000;
    pInfo->hint_interval[POWER_HINT_APP_LAUNCH] = 2000000;
    pInfo->hint_interval[POWER_HINT_SHIELD_STREAMING] = 500000;
    pInfo->hint_interval[POWER_HINT_HIGH_RES_VIDEO] = 500000;
    pInfo->hint_interval[POWER_HINT_MIRACAST] = 500000;

    // Initialize AppProfile defaults
    pInfo->defaults.min_freq = 0;
    pInfo->defaults.fan_cap = AP_FAN_STATE_CAP_DEFAULT;
    pInfo->defaults.volt_temp_mode = AP_VOLT_TEMP_MODE_DEFAULT;
    pInfo->defaults.power_cap = 0;
    pInfo->defaults.gpu_cap = UINT_MAX;

    sysfs_read(EDP_MODE_NODE,
                buf, size);
    pInfo->defaults.edp_mode = (char*)malloc(strlen(buf));
    strcpy(pInfo->defaults.edp_mode, buf);

    pInfo->defaults.pmu_mode = DEFAULT_PMU_MODE;
    pInfo->defaults.host1x_floor_state = 0;
    pInfo->defaults.emc_floor_state = 0;
    pInfo->defaults.cbus_floor_state = 0;
    pInfo->defaults.sdhci_boost_state = 0;

    // Initialize fds
    pInfo->fds.app_min_freq = -1;

    // Initialize features
    pInfo->features.fan = sysfs_exists("/sys/devices/platform/pwm-fan/state_cap");
    pInfo->features.edp_mode = sysfs_exists(EDP_MODE_NODE);
    pInfo->features.volt_temp_mode =
        sysfs_exists("/sys/module/edp/parameters/edp_volt_temp_mode");

    free(buf);
}

static bool get_property_bool(const char *key, bool default_value)
{
    char value[PROPERTY_VALUE_MAX];

    if (property_get(key, value, NULL) > 0) {
        if (!strcmp(value, "1") || !strcasecmp(value, "on") ||
            !strcasecmp(value, "true")) {
            return true;
        }
        if (!strcmp(value, "0") || !strcasecmp(value, "off") ||
            !strcasecmp(value, "false")) {
            return false;
        }
    }

    return default_value;
}

static void set_property_int(const char *key, int value)
{
    char val[32];
    int status;

    snprintf(val, sizeof(val), "%d", value);
    status = property_set(key, val);

    if (status) {
        ALOGE("Error writing to property: %s\n", key);
    }
}

static void sysfs_write_int(const char *path, int value)
{
    char val[32];

    snprintf(val, sizeof(val), "%d", value);
    sysfs_write(path, val);
}

static void set_cpu_scaling_min_freq(struct powerhal_info *pInfo, int value)
{
    if (value < 0)
        value = pInfo->defaults.min_freq;

    if (pInfo->fds.app_min_freq >= 0) {
        close(pInfo->fds.app_min_freq);
        pInfo->fds.app_min_freq = -1;
    }

    pInfo->fds.app_min_freq = pInfo->mTimeoutPoker->requestPmQos("/dev/cpu_freq_min", value);
}

static void set_gpu_scaling(struct powerhal_info *pInfo, int value)
{
    if (value)
        value = 1;

    sysfs_write_int("/sys/devices/platform/host1x/gr3d/enable_3d_scaling", value);
}

static void set_gpu_core_cap(struct powerhal_info *pInfo, int value)
{
    if (value < 0)
        value = pInfo->defaults.gpu_cap;

    sysfs_write_int("sys/kernel/tegra_cap/cbus_cap_state", 1);
    sysfs_write_int("sys/kernel/tegra_cap/cbus_cap_level", value);
}

static void set_edp_mode(struct powerhal_info *pInfo, int value)
{
    if (!pInfo->features.edp_mode)
        return;

    // Transition only if edp state changed
    if (value < 0 && old_edp_state >= 0) {
        sysfs_write_int(PMU_MODE_NODE, pInfo->defaults.pmu_mode);
        sysfs_write_int(HOST1X_FLOOR_NODE, pInfo->defaults.host1x_floor_state);
        sysfs_write(EDP_MODE_NODE, pInfo->defaults.edp_mode);
        sysfs_write_int(CBUS_FLOOR_NODE, pInfo->defaults.cbus_floor_state);
        sysfs_write_int(SDHCI_BOOST_NODE, pInfo->defaults.sdhci_boost_state);
        sysfs_write_int(EMC_FLOOR_NODE, pInfo->defaults.emc_floor_state);
    } else if (value >= 0 && old_edp_state < 0) {
        sysfs_write_int(EMC_FLOOR_NODE, !pInfo->defaults.emc_floor_state);
        sysfs_write_int(CBUS_FLOOR_NODE, !pInfo->defaults.cbus_floor_state);
        sysfs_write_int(SDHCI_BOOST_NODE, !pInfo->defaults.sdhci_boost_state);
        sysfs_write(EDP_MODE_NODE, "profile_favor_gpu\r\n");
        sysfs_write_int(HOST1X_FLOOR_NODE, !pInfo->defaults.host1x_floor_state);
        sysfs_write_int(PMU_MODE_NODE, THREE_PHASE_PMU_MODE);
    }
    old_edp_state = value;
}

static void set_pbc_power(struct powerhal_info *pInfo, int value)
{
    if (value < 0)
        value = pInfo->defaults.power_cap;

    set_property_int(POWER_CAP_PROP, value);
}

static void set_fan_cap(struct powerhal_info *pInfo, int value)
{
    if (!pInfo->features.fan)
        return;

    if (value < 0)
        value = pInfo->defaults.fan_cap;

    sysfs_write_int("/sys/devices/platform/pwm-fan/state_cap", value);
}

static void set_volt_temp_mode(struct powerhal_info *pInfo, int value)
{
    if (!pInfo->features.volt_temp_mode)
        return;

    if (value < 0)
        value = pInfo->defaults.volt_temp_mode;

    sysfs_write("/sys/module/edp/parameters/edp_volt_temp_mode", value ? "Y" : "N");
}

static void app_profile_set(struct powerhal_info *pInfo, app_profile_knob *data)
{
    int i;

    for (i = 0; i < APP_PROFILE_COUNT; i++)
    {
        switch (i) {
            case APP_PROFILE_CPU_SCALING_MIN_FREQ:
                set_cpu_scaling_min_freq(pInfo, data[i]);
                break;
            case APP_PROFILE_GPU_CBUS_CAP_LEVEL:
                set_gpu_core_cap(pInfo, data[i]);
                break;
            case APP_PROFILE_GPU_SCALING:
                set_gpu_scaling(pInfo, data[i]);
                break;
            case APP_PROFILE_EDP_MODE:
                set_edp_mode(pInfo, data[i]);
                break;
            case APP_PROFILE_PBC_POWER:
                set_pbc_power(pInfo, data[i]);
                break;
            case APP_PROFILE_FAN_CAP:
                set_fan_cap(pInfo, data[i]);
            case APP_PROFILE_VOLT_TEMP_MODE:
                set_volt_temp_mode(pInfo, data[i]);
            default:
                break;
        }
    }
}

void common_power_init(struct power_module *module, struct powerhal_info *pInfo)
{
    common_power_open(pInfo);

    pInfo->ftrace_enable = get_property_bool("nvidia.hwc.ftrace_enable", false);

    // Boost to max frequency on initialization to decrease boot time
    pInfo->mTimeoutPoker->requestPmQosTimed("/dev/cpu_freq_min",
                                     pInfo->available_frequencies[pInfo->num_available_frequencies - 1],
                                     s2ns(15));
}

void common_power_set_interactive(struct power_module *module, struct powerhal_info *pInfo, int on)
{
    int i;
    int dev_id;
    char path[80];
    const char* state = (0 == on)?"0":"1";

    set_led_state(on);
    sysfs_write("/sys/devices/platform/host1x/nvavp/boost_sclk", state);
    sysfs_write("/sys/class/leds/roth-led/enable", state);
    if (0 != pInfo) {
        for (i = 0; i < pInfo->input_cnt; i++) {
            if (0 == pInfo->input_devs)
                dev_id = i;
            else if (-1 == pInfo->input_devs[i].dev_id)
                continue;
            else
                dev_id = pInfo->input_devs[i].dev_id;
            sprintf(path, "/sys/class/input/input%d/enabled", dev_id);
            if (!access(path, W_OK)) {
                if (0 == on)
                    ALOGI("Disabling input device:%d", dev_id);
                else
                    ALOGI("Enabling input device:%d", dev_id);
                sysfs_write(path, state);
            }
        }
    }

    sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor",
        (on == 0)?"conservative":"interactive");
}

void common_power_hint(struct power_module *module, struct powerhal_info *pInfo,
                            power_hint_t hint, void *data)
{
    uint64_t t;

    if (!pInfo)
        return;

    if (check_hint(pInfo, hint, &t) < 0)
        return;

    switch (hint) {
    case POWER_HINT_VSYNC:
        break;
    case POWER_HINT_INTERACTION:
        if (pInfo->ftrace_enable) {
            sysfs_write("/sys/kernel/debug/tracing/trace_marker", "Start POWER_HINT_INTERACTION\n");
        }
        // Boost to max lp frequency
        pInfo->mTimeoutPoker->requestPmQosTimed("/dev/cpu_freq_min",
                                                 pInfo->lp_max_frequency,
                                                 s2ns(1));
        break;
    case POWER_HINT_APP_LAUNCH:
        // Boost to 1.2Ghz dual core
        pInfo->mTimeoutPoker->requestPmQosTimed("dev/cpu_freq_min",
                                                 1200000,
                                                 s2ns(2));
        pInfo->mTimeoutPoker->requestPmQosTimed("dev/min_online_cpus",
                                                 2,
                                                 s2ns(2));
        break;
    case POWER_HINT_APP_PROFILE:
        if (data) {
            app_profile_set(pInfo, (app_profile_knob*)data);
        }
        break;
    case POWER_HINT_SHIELD_STREAMING:
        // Boost to 816 Mhz frequency for one second
        pInfo->mTimeoutPoker->requestPmQosTimed("/dev/cpu_freq_min",
                                                 816000,
                                                 s2ns(1));
        break;
    case POWER_HINT_HIGH_RES_VIDEO:
        // Boost to max LP frequency for one second
        pInfo->mTimeoutPoker->requestPmQosTimed("/dev/cpu_freq_min",
                                                 pInfo->lp_max_frequency,
                                                 s2ns(1));
        break;
    case POWER_HINT_MIRACAST:
        // Boost to 816 Mhz frequency for one second
        pInfo->mTimeoutPoker->requestPmQosTimed("/dev/cpu_freq_min",
                                                 816000,
                                                 s2ns(1));
        break;
    default:
        break;
    }

    pInfo->hint_time[hint] = t;
}

