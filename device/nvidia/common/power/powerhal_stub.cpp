/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "powerhal.h"

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

void common_power_open(struct powerhal_info *pInfo)
{
}

void common_power_init(struct power_module *module, struct powerhal_info *pInfo)
{
}

void common_power_set_interactive(struct power_module *module, struct powerhal_info *pInfo, int on)
{
}

void common_power_hint(struct power_module *module, struct powerhal_info *pInfo,
                            power_hint_t hint, void *data)
{
}

