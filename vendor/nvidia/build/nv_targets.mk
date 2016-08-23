#
# Nvidia specific targets
#

.PHONY: dev nv-blob sim-image

dev: droidcore target-files-package
ifneq ($(NO_ROOT_DEVICE),)
ifeq ($(TARGET_BOARD_PLATFORM_TYPE),simulation)
	device/nvidia/common/generate_full_filesystem.sh
else
	device/nvidia/common/generate_nvtest_ramdisk.sh $(TARGET_PRODUCT) $(TARGET_BUILD_TYPE)
	device/nvidia/common/generate_qt_ramdisk.sh $(TARGET_PRODUCT) $(TARGET_BUILD_TYPE)
endif
endif

#
# Generate ramdisk images for simulation
#
sim-image: nvidia-tests
	device/nvidia/common/copy_simtools.sh
	device/nvidia/common/generate_full_filesystem.sh

#
# bootloader blob target and macros
#

# macro: checks file existence and returns list of existing file
# $(1) list of file paths
define _dynamic_blob_dependencies
$(foreach f,$(1), $(eval \
 ifneq ($(wildcard $(f)),)
  _dep += $(f)
 endif))\
 $(_dep)
 $(eval _dep :=)
endef

# macro: construct command line for nvblob based on type of input file
# $(1) list of file paths
define _blob_command_line
$(foreach f,$(1), $(eval \
 ifneq ($(filter %microboot.bin,$(f)),)
  _cmd += $(f) NVC 1
  _cmd += $(f) RMB 1
 else ifneq ($(filter %.dtb,$(f)),)
  _cmd += $(f) DTB 1
 else ifneq ($(filter %.bct,$(f)),)
  _cmd += $(f) BCT 1
 else ifneq ($(filter %xusb_sil_rel_fw,$(f)),)
  _cmd += $(f) DFI 1
 else ifneq ($(filter %charging.bmp,$(f)),)
  _cmd += $(f) CHG 1
 else ifneq ($(filter %charged.bmp,$(f)),)
  _cmd += $(f) FBP 1
 else ifneq ($(filter %lowbat.bmp,$(f)),)
  _cmd += $(f) LBP 1
 else ifneq ($(filter %nvidia.bmp,$(f)),)
  _cmd += $(f) BMP 1
 endif))\
 $(_cmd)
 $(eval _cmd :=)
endef

# These are additional files for which we generate blobs only if they exists
_blob_deps := \
      $(PRODUCT_OUT)/microboot.bin \
      $(PRODUCT_OUT)/$(TARGET_KERNEL_DT_NAME).dtb \
      $(PRODUCT_OUT)/flash.bct \
      $(PRODUCT_OUT)/xusb_sil_rel_fw \
      $(PRODUCT_OUT)/charged.bmp \
      $(PRODUCT_OUT)/charging.bmp \
      $(PRODUCT_OUT)/lowbat.bmp \
      $(PRODUCT_OUT)/nvidia.bmp

# target to generate blob
nv-blob: \
      $(HOST_OUT_EXECUTABLES)/nvblob \
      $(HOST_OUT_EXECUTABLES)/nvsignblob \
      $(TOP)/device/nvidia/common/security/signkey.pk8 \
      $(PRODUCT_OUT)/bootloader.bin \
      $(call _dynamic_blob_dependencies, $(_blob_deps))
	$(hide) python $(filter %nvblob,$^) \
		$(filter %bootloader.bin,$^) EBT 1 \
		$(filter %bootloader.bin,$^) RBL 1 \
		 $(call _blob_command_line, $^)

# Clear local variable
_blob_deps :=
