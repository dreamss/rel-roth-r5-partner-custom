LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libtbb
LOCAL_MODULE_TAGS := optional

LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/src \
        $(LOCAL_PATH)/src/rml/include

LOCAL_CFLAGS := -DANDROID -DTBB_USE_GCC_BUILTINS=1 -D__TBB_DYNAMIC_LOAD_ENABLED=0 \
                -D__TBB_BUILD=1 -D__TBB_SURVIVE_THREAD_SWITCH=0 -DUSE_PTHREAD \
                -DTBB_USE_DEBUG=0 -DTBB_NO_LEGACY=1 -DDO_ITT_NOTIFY=0 \
                -fsigned-char -fdata-sections -ffunction-sections -frtti -fexceptions \
                -fweb -fwrapv -frename-registers -fsched2-use-superblocks -fsched2-use-traces \
                -fsched-stalled-insns-dep=100 -fsched-stalled-insns=2 \
                -fdiagnostics-show-option -fomit-frame-pointer -mthumb -O3 \
                -include $(LOCAL_PATH)/android_additional.h -Wno-non-virtual-dtor

TBB_SRC_FILES := $(wildcard $(LOCAL_PATH)/src/tbb/*.cpp)
LOCAL_SRC_FILES := $(TBB_SRC_FILES:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES += src/rml/client/rml_tbb.cpp

LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wcast-align -Wundef -Wmissing-declarations

include $(NVIDIA_SHARED_LIBRARY)
