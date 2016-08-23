# NVIDIA Tegra4 "Dalmore" development system
#
# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.


BOARD_USES_SHIELDTECH := true

# Add support for Controller menu
PRODUCT_COPY_FILES += \
    vendor/nvidia/shieldtech/common/etc/com.nvidia.shieldtech.xml:system/etc/permissions/com.nvidia.shieldtech.xml

# RSMouse
ifneq ($(SHIELDTECH_FEATURE_RSMOUSE),false)
TARGET_GLOBAL_CPPFLAGS += -DSHIELDTECH_RSMOUSE
TARGET_GLOBAL_CFLAGS += -DSHIELDTECH_RSMOUSE
endif


# Controller-based Keyboard
ifneq ($(SHIELDTECH_FEATURE_KEYBOARD),false)
PRODUCT_PACKAGES += \
  NVLatinIME \
  libjni_nvlatinime
endif

# Full-screen Mode
ifneq ($(SHIELDTECH_FEATURE_FULLSCREEN),false)
DEVICE_PACKAGE_OVERLAYS += vendor/nvidia/shieldtech/common/feature_overlays/fullscreen_mode
endif

# Console Mode
ifneq ($(SHIELDTECH_FEATURE_CONSOLE_MODE),false)
DEVICE_PACKAGE_OVERLAYS += vendor/nvidia/shieldtech/common/feature_overlays/console_mode
PRODUCT_PACKAGES += \
  ConsoleUI \
  ConsoleSplash
endif

# Blake controller
ifneq ($(SHIELDTECH_FEATURE_BLAKE),false)
DEVICE_PACKAGE_OVERLAYS += vendor/nvidia/shieldtech/common/feature_overlays/blake
PRODUCT_PACKAGES += \
  blake \
  lota
endif

# NvAndroidOSC
ifneq ($(SHIELDTECH_FEATURE_OSC),false)
DEVICE_PACKAGE_OVERLAYS += vendor/nvidia/shieldtech/common/feature_overlays/android_osc
PRODUCT_PACKAGES += \
  NvAndroidOSC
endif

# Gallery
ifeq ($(SHIELDTECH_FEATURE_NVGALLERY),true)
PRODUCT_PACKAGES += \
  NVGallery \
  libnvjni_eglfence \
  libnvjni_filtershow_filters \
  libnvjni_mosaic
endif

# Generic ShieldTech Features
DEVICE_PACKAGE_OVERLAYS += vendor/nvidia/shieldtech/common/overlay
