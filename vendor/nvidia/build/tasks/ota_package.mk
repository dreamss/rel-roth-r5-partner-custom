#
# This file is included from $TOP/build/core/Makefile
# It has those variables available which are set from above Makefile
#


#
# Override OTA update package target (run with -n)
# Used for developer OTA packages which legitimately need to go back and forth
#
$(INTERNAL_OTA_PACKAGE_TARGET): $(BUILT_TARGET_FILES_PACKAGE) $(DISTTOOLS)
	@echo "Package Dev OTA: $@"
	$(hide) $(TOP)/build/tools/releasetools/ota_from_target_files -n -v \
	   -p $(HOST_OUT) \
	   -k $(KEY_CERT_PAIR) \
	   $(BUILT_TARGET_FILES_PACKAGE) $@

