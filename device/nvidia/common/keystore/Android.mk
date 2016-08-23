# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
ifeq ($(NV_ANDROID_FRAMEWORK_ENHANCEMENTS),TRUE)
ifeq ($(SECURE_OS_BUILD),y)
LOCAL_PATH := $(call my-dir)

ifneq ($(TARGET_SIMULATOR),true)

include $(NVIDIA_DEFAULTS)

# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.board.platform>.so

LOCAL_MODULE := keystore.tegra
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SRC_FILES := keymaster.cpp

LOCAL_C_INCLUDES := \
»       3rdparty/trustedlogic/sdk/tegra4/tf_sdk/include \
»       3rdparty/trustedlogic/sdk/tegra4/tegra4_secure_world_integration_kit/sddk/include \
»       $(TEGRA_TOP)/core/include \
»       external/openssl/include \
»       system/security/keystore

LOCAL_C_FLAGS = -fvisibility=hidden -Wall #-Werror

LOCAL_SHARED_LIBRARIES := liblog libcrypto libtf_crypto_sst

LOCAL_MODULE_TAGS := optional

LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1

include $(NVIDIA_SHARED_LIBRARY)

endif # !TARGET_SIMULATOR
endif # SECURE_OS_BUILD
endif # NV_ANDROID_FRAMEWORK_ENHANCEMENTS
