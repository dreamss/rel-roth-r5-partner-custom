/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 *
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

#ifndef COMMON_POWER_HAL_H
#define COMMON_POWER_HAL_H

#include <hardware/hardware.h>
#include <hardware/power.h>
#include <cutils/properties.h>

#include "timeoutpoker.h"

#define MAX_CHARS 32
#define MAX_INPUT_DEV_COUNT 12

struct input_dev_map {
    int dev_id;
    const char* dev_name;
};

struct powerhal_info {
    TimeoutPoker* mTimeoutPoker;

    int *available_frequencies;
    int num_available_frequencies;

    /* Maximum LP CPU frequency */
    int lp_max_frequency;

    bool ftrace_enable;

    /* Number of devices requesting Power HAL service */
    int input_cnt;

    /* Holds input devices */
    struct input_dev_map* input_devs;

    /* Time last hint was sent - in usec */
    uint64_t hint_time[POWER_HINT_COUNT];
    uint64_t hint_interval[POWER_HINT_COUNT];

    /* AppProfile defaults */
    struct {
        int min_freq;
        int gpu_cap;
        char *edp_mode;
        int pmu_mode;
        int host1x_floor_state;
        int emc_floor_state;
        int cbus_floor_state;
        int sdhci_boost_state;
        int fan_cap;
        int volt_temp_mode;
        int power_cap;
    } defaults;

    /* Features on platform */
    struct {
        bool fan;
        bool edp_mode;
        bool volt_temp_mode;
    } features;

    /* File descriptors used for hints and app profiles */
    struct {
        int app_min_freq;
    } fds;
};

/* Write to sysfs on mentioned path with value s */
void sysfs_write(const char *path, const char *s);

/* Read from sysfs on mentioned path with value s */
void sysfs_read(const char *path, char *s, int size);

/* Verify that there exists a sysfs node at path */
bool sysfs_exists(const char *path);

/* Returns true if frequency is in available_frequencies */
bool is_available_frequency(struct powerhal_info *pInfo, int freq);

/* Opens power hw module */
void common_power_open(struct powerhal_info *pInfo);

/* Power management setup action at startup.
 * Such as to set default cpufreq parameters.
 */
void common_power_init(struct power_module *module, struct powerhal_info *pInfo);

/* Power management action,
 * upon the system entering interactive state and ready for interaction,
 * often with UI devices
 * OR
 * non-interactive state the system appears asleep, displayi/touch usually turned off.
*/
void common_power_set_interactive(struct power_module *module,
                                    struct powerhal_info *pInfo, int on);

/* PowerHint called to pass hints on power requirements, which
 * may result in adjustment of power/performance parameters of the
 * cpufreq governor and other controls.
*/
void common_power_hint(struct power_module *module, struct powerhal_info *pInfo,
                            power_hint_t hint, void *data);

#endif  //COMMON_POWER_HAL_H

