ifneq ($(filter-out SHARED_LIBRARIES EXECUTABLES,$(LOCAL_MODULE_CLASS)),)
$(error The integration layer for the nvmake build system supports only shared libraries and executables)
endif

include $(NVIDIA_BASE)
# TODO Enable coverage build

ifeq ($(LOCAL_NVIDIA_NVMAKE_OVERRIDE_BUILD_TYPE),)
  NVIDIA_NVMAKE_BUILD_TYPE := $(TARGET_BUILD_TYPE)
else
  NVIDIA_NVMAKE_BUILD_TYPE := $(LOCAL_NVIDIA_NVMAKE_OVERRIDE_BUILD_TYPE)
endif

ifeq ($(NV_MOBILE_DGPU),1)
  # Using this library path causes build failures on Ubuntu 12.04, but
  # dGPU builds require it.
  NVIDIA_NVMAKE_LIBRARY_PATH := $(P4ROOT)/sw/mobile/tools/linux/android/nvmake/unix-build/lib
else
  NVIDIA_NVMAKE_LIBRARY_PATH := $(LD_LIBRARY_PATH)
endif

ifeq ($(LOCAL_NVIDIA_NVMAKE_OVERRIDE_MODULE_NAME),)
  NVIDIA_NVMAKE_MODULE_NAME := $(LOCAL_MODULE)
else
  NVIDIA_NVMAKE_MODULE_NAME := $(LOCAL_NVIDIA_NVMAKE_OVERRIDE_MODULE_NAME)
endif

ifeq ($(LOCAL_NVIDIA_NVMAKE_OVERRIDE_TOP),)
  NVIDIA_NVMAKE_TOP := $(TEGRA_TOP)/gpu/$(LOCAL_NVIDIA_NVMAKE_TREE)
else
  NVIDIA_NVMAKE_TOP := $(LOCAL_NVIDIA_NVMAKE_OVERRIDE_TOP)
endif

ifeq ($(LOCAL_NVIDIA_NVMAKE_OVERRIDE_TARGET_ABI),)
  NVIDIA_NVMAKE_TARGET_ABI := androideabi
else
  NVIDIA_NVMAKE_TARGET_ABI := $(LOCAL_NVIDIA_NVMAKE_OVERRIDE_TARGET_ABI)
endif

ifeq ($(LOCAL_NVIDIA_NVMAKE_OVERRIDE_TARGET_OS),)
  NVIDIA_NVMAKE_TARGET_OS := Android
else
  NVIDIA_NVMAKE_TARGET_OS := $(LOCAL_NVIDIA_NVMAKE_OVERRIDE_TARGET_OS)
endif

NVIDIA_NVMAKE_MODULE_PRIVATE_PATH := $(LOCAL_NVIDIA_NVMAKE_OVERRIDE_MODULE_PRIVATE_PATH)

NVIDIA_NVMAKE_MODULE := \
    $(NVIDIA_NVMAKE_TOP)/$(LOCAL_NVIDIA_NVMAKE_BUILD_DIR)/_out/$(NVIDIA_NVMAKE_TARGET_OS)_ARMv7_$(NVIDIA_NVMAKE_TARGET_ABI)_$(NVIDIA_NVMAKE_BUILD_TYPE)/$(NVIDIA_NVMAKE_MODULE_PRIVATE_PATH)/$(NVIDIA_NVMAKE_MODULE_NAME)$(LOCAL_MODULE_SUFFIX)

ifneq ($(strip $(SHOW_COMMANDS)),)
  NVIDIA_NVMAKE_VERBOSE := NV_VERBOSE=1
else
  NVIDIA_NVMAKE_VERBOSE := -s
endif

# extra definitions to pass to nvmake
NVIDIA_NVMAKE_EXTRADEFS :=

#
# Call into the nvmake build system to build the module
#

$(NVIDIA_NVMAKE_MODULE) $(LOCAL_MODULE)_nvmakeclean: NVIDIA_NVMAKE_COMMAND := $(MAKE) \
    MAKE=$(shell which $(MAKE)) \
    LD_LIBRARY_PATH=$(NVIDIA_NVMAKE_LIBRARY_PATH) \
    NV_ANDROID_TOOLS=$(P4ROOT)/sw/mobile/tools/linux/android/nvmake \
    NV_UNIX_BUILD_CHROOT=$(P4ROOT)/sw/tools/unix/hosts/Linux-x86/unix-build \
    NV_SOURCE=$(NVIDIA_NVMAKE_TOP) \
    NV_TOOLS=$(P4ROOT)/sw/tools \
    NV_HOST_OS=Linux \
    NV_HOST_ARCH=x86 \
    NV_TARGET_OS=$(NVIDIA_NVMAKE_TARGET_OS) \
    NV_TARGET_ARCH=ARMv7 \
    NV_BUILD_TYPE=$(NVIDIA_NVMAKE_BUILD_TYPE) \
    $(NVIDIA_NVMAKE_EXTRADEFS) \
    $(NVIDIA_NVMAKE_VERBOSE) \
    -C $(NVIDIA_NVMAKE_TOP)/$(LOCAL_NVIDIA_NVMAKE_BUILD_DIR) \
    -f makefile.nvmk \
    $(LOCAL_NVIDIA_NVMAKE_ARGS)

$(NVIDIA_NVMAKE_MODULE) $(LOCAL_MODULE)_nvmakeclean: NVIDIA_NVMAKE_RM_MODULE_MAKE := $(MAKE) \
    MAKE=$(shell which $(MAKE)) \
    PATH=$(ARM_EABI_TOOLCHAIN):/usr/bin/:$(PATH) \
    CROSS_COMPILE=arm-eabi- \
    ARCH=arm \
    SYSOUT=$(ANDROID_BUILD_TOP)/$(TARGET_OUT_INTERMEDIATES)/KERNEL/ \
    SYSSRC=$(ANDROID_BUILD_TOP)/kernel/ \
    CC=$(ARM_EABI_TOOLCHAIN)/arm-eabi-gcc \
    LD=$(ARM_EABI_TOOLCHAIN)/arm-eabi-ld \
    NV_MOBILE_DGPU=$(NV_MOBILE_DGPU) \
    -C $(dir $(NVIDIA_NVMAKE_MODULE)) -f makefile nv-linux.o

ifeq ($(LOCAL_NVIDIA_NVMAKE_BUILD_DIR), drivers/resman)
  $(NVIDIA_NVMAKE_MODULE): NVIDIA_NVMAKE_POST_BUILD_COMMAND := \
    cd $(dir $(NVIDIA_NVMAKE_MODULE)); \
    $(MAKE) MAKE=$(shell which $(MAKE)) -C $(dir $(NVIDIA_NVMAKE_MODULE)) -f makefile clean; \
    $(NVIDIA_NVMAKE_RM_MODULE_MAKE)
else
  $(NVIDIA_NVMAKE_MODULE): NVIDIA_NVMAKE_POST_BUILD_COMMAND :=
endif

# This target needs to be forced, nvmake will do its own dependency checking
$(NVIDIA_NVMAKE_MODULE): $(LOCAL_ADDITIONAL_DEPENDENCIES) FORCE
	@echo "Build with nvmake: $(PRIVATE_MODULE) ($@)"
	+$(hide) $(NVIDIA_NVMAKE_COMMAND)
	+$(hide) $(NVIDIA_NVMAKE_POST_BUILD_COMMAND)

$(LOCAL_MODULE)_nvmakeclean:
	@echo "Clean nvmake build files: $(PRIVATE_MODULE)"
	+$(hide) $(NVIDIA_NVMAKE_COMMAND) clobber

.PHONY: $(LOCAL_MODULE)_nvmakeclean

#
# Bring module from the nvmake build output, and apply the usual
# processing for shared library or executable.
# Also make the module's clean target descend into nvmake.
#

include $(BUILD_SYSTEM)/dynamic_binary.mk

$(linked_module): $(NVIDIA_NVMAKE_MODULE) | $(ACP)
	@echo "Copy from nvmake output: $(PRIVATE_MODULE) ($@)"
	$(copy-file-to-target)

ifeq ($(LOCAL_NVIDIA_NVMAKE_BUILD_DIR), drivers/resman)

$(strip_output): NVIDIA_NVMAKE_RM_KERNEL_INTERFACE_PATH := \
    $(NVIDIA_NVMAKE_TOP)/$(LOCAL_NVIDIA_NVMAKE_BUILD_DIR)/_out/$(NVIDIA_NVMAKE_TARGET_OS)_ARMv7_$(NVIDIA_NVMAKE_TARGET_ABI)_$(NVIDIA_NVMAKE_BUILD_TYPE)/$(NVIDIA_NVMAKE_MODULE_PRIVATE_PATH)

$(strip_output): NVIDIA_NVMAKE_RM_STRIP := \
    $(ARM_EABI_TOOLCHAIN)/arm-eabi-strip -g

NVIDIA_NVMAKE_RM_INSTALLER_OBJ_FILES := \
    $(TARGET_OUT_VENDOR)/nvidia/dgpu/nv-kernel.o \
    $(TARGET_OUT_VENDOR)/nvidia/dgpu/nv-linux.o \
    $(TARGET_OUT_VENDOR)/nvidia/dgpu/nvidia.mod.o

$(strip_output): $(NVIDIA_NVMAKE_RM_INSTALLER_OBJ_FILES) \
    $(TARGET_OUT_VENDOR)/nvidia/dgpu/module-common.lds \
    $(TARGET_OUT_EXECUTABLES)/makedevices.sh \
    $(TARGET_OUT_EXECUTABLES)/nv-dgpu-install.sh \
    $(TARGET_OUT_EXECUTABLES)/arm-android-eabi-ld

$(strip_output): $(linked_module) | $(ACP)
	@echo "target Stripped: $(PRIVATE_MODULE) ($@)"
	$(copy-file-to-target)
	+$(hide) $(ACP) -fp $@ $@.sym
	+$(hide) $(NVIDIA_NVMAKE_RM_STRIP) $@

# Define a rule to copy and strip rm obj file.  For use via $(eval).
# $(1): file to copy
define copy-and-strip-rm-obj-file
$(1): $(NVIDIA_NVMAKE_MODULE) | $(ACP)
	@echo "target Stripped: $(notdir $(1)) ($(1))"
	@mkdir -p $$(TARGET_OUT_VENDOR)/nvidia/dgpu
	+$(hide) $(ACP) -fp $$(NVIDIA_NVMAKE_RM_KERNEL_INTERFACE_PATH)/$(notdir $(1)) $(1)
	+$(hide) $$(NVIDIA_NVMAKE_RM_STRIP) $(1)
endef

$(foreach file,$(NVIDIA_NVMAKE_RM_INSTALLER_OBJ_FILES), \
    $(eval $(call copy-and-strip-rm-obj-file,$(file))))

$(TARGET_OUT_VENDOR)/nvidia/dgpu/module-common.lds: $(TOP)/kernel/scripts/module-common.lds | $(ACP)
	@echo "Copy $< => $@"
	+$(hide) $(copy-file-to-target)

$(TARGET_OUT_EXECUTABLES)/makedevices.sh: $(NVIDIA_NVMAKE_TOP)/$(LOCAL_NVIDIA_NVMAKE_BUILD_DIR)/arch/nvalloc/unix/Linux/makedevices.sh | $(ACP)
	@echo "Copy $< => $@"
	+$(hide) $(copy-file-to-target)

$(TARGET_OUT_EXECUTABLES)/arm-android-eabi-ld: $(TEGRA_TOP)/3rdparty/binutils/bin/arm-android-eabi-ld | $(ACP)
	@echo "Copy $< => $@"
	+$(hide) $(copy-file-to-target)

$(TARGET_OUT_EXECUTABLES)/nv-dgpu-install.sh: $(TEGRA_TOP)/core/drivers/nvrm/dgpu/nv-dgpu-install.sh | $(ACP)
	@echo "Copy $< => $@"
	+$(hide) $(copy-file-to-target)

endif

$(cleantarget):: $(LOCAL_MODULE)_nvmakeclean

NVIDIA_NVMAKE_BUILD_TYPE :=
NVIDIA_NVMAKE_TOP :=
NVIDIA_NVMAKE_LIBRARY_PATH :=
NVIDIA_NVMAKE_MODULE :=
NVIDIA_NVMAKE_MODULE_NAME :=
NVIDIA_NVMAKE_VERBOSE :=
NVIDIA_NVMAKE_TARGET_ABI :
NVIDIA_NVMAKE_TARGET_OS :=
NVIDIA_NVMAKE_MODULE_PRIVATE_PATH :=
NVIDIA_NVMAKE_RM_KERNEL_INTERFACE_PATH :=
NVIDIA_NVMAKE_RM_INSTALLER_OBJ_FILES :=
