# Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
# Build definitions common to all NVIDIA boards.

# If during build configuration setup i.e. during choosecombo or lunch or
# using $TOP/buildspec.mk TARGET_PRODUCT is set to one of Nvidia boards then
# REFERENCE_DEVICE is the same as TARGET_DEVICE. For boards derived from 
# NVIDIA boards, REFERENCE_DEVICE should be set to the NVIDIA
# reference device name in BoardConfig.mk or in the shell environment.

REFERENCE_DEVICE ?= $(TARGET_DEVICE)

# common specific sepolicy
BOARD_SEPOLICY_DIRS := device/nvidia/common/sepolicy/

BOARD_SEPOLICY_UNION := healthd.te \
	netd.te \
	installd.te \
	untrusted_app.te \
	vold.te
