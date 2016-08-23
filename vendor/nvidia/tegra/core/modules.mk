# DO NOT add conditionals to this makefile of the form
#
#    ifeq ($(TARGET_TEGRA_VERSION),<latest SOC>)
#        <stuff for latest SOC>
#    endif
#
# Such conditionals break forward compatibility with future SOCs.
# If you must add conditionals to this makefile, use the form
#
#    ifneq ($(filter <list of older SOCs>,$(TARGET_TEGRA_VERSION)),)
#       <stuff for old SOCs>
#    else
#       <stuff for new SOCs>
#    endif

nv_modules := \
    alcameradatataptest \
    aleglstream \
    alplaybacktest \
    alsnapshottest \
    alvcplayer \
    alvcrecorder \
    alvideorecordtest \
    alvepreview \
    bmp2h \
    bootloader \
    btmacwriter\
    buildbct \
    com.nvidia.display \
    com.nvidia.nvstereoutils \
    dfs_cfg \
    dfs_log \
    dfs_monitor \
    dfs_stress \
    DidimCalibration \
    fuse_bypass.txt \
    get_fs_stat \
    gps.$(TARGET_BOARD_PLATFORM) \
    hdcp_test \
    init.tf \
    inv_self_test \
    libaudioutils \
    libexpat \
    libtinyalsa \
    libcg \
    libcgdrv \
    libDEVMGR \
    libhybrid \
    libinvensense_hal \
    libmd5 \
    libmllite \
    libmplmpu \
    libnv3p \
    libnv3pserver \
    libnvaboot \
    libnvaes_ref \
    libnvappmain \
    libnvappmain_aos \
    libnvapputil \
    libnvbct \
    libnvboothost \
    libnvbootupdate \
    libnvcpl \
    libnvcpud \
    libpowerservice \
    libnvcrypto \
    libnvddk_aes \
    libnvddk_audiodap \
    libnvddk_disp \
    libnvddk_fuse \
    libnvddk_fuse_read_avp \
    libnvddk_fuse_read_host \
    libnvddk_i2s \
    libnvddk_misc \
    libnvddk_spdif \
    libnvdioconverter \
    libnvdispatch_helper \
    libnvdtvsrc \
    libnvfs \
    libnvfsmgr \
    libnvfusebypass \
    libnvfxmath \
    libnvhdmi3dplay_jni \
    libnvidia_display_jni \
    libnvimageio \
    libnvintr \
    libnvmm \
    libnvmm_ac3audio \
    libnvmm_audio \
    libnvmm_image \
    libnvmm_msaudio \
    libnvodm_audiocodec \
    libnvodm_dtvtuner \
    libnvodm_hdmi \
    libnvodm_misc \
    libnvodm_query \
    libnvodm_services \
    libnvopt_dvm \
    libnvos \
    libnvos_aos \
    libnvos_aos_libgcc_avp \
    libnvpartmgr \
    libnvreftrack \
    libnvrm \
    libnvrm_channel_impl \
    libnvrm_graphics \
    libnvrm_impl \
    libnvrm_limits \
    libnvrm_secure \
    libnvrsa \
    libnvseaes_keysched_lock_avp \
    libnvstormgr \
    libnvsystemuiext_jni \
    libnvsystem_utils \
    libnvtestio \
    libnvtestmain \
    libnvtestresults \
    libnvtestrun \
    libnvtsdemux \
    libnvusbhost \
    libopengles1_detect \
    libopengles2_detect \
    libopenmaxil_detect \
    libopenvg_detect \
    librs_jni \
    libsense_fu \
    libsensors.base \
    libsensors.isl29018 \
    libsensors.isl29028 \
    libsensors.mpl \
    librm31080 \
    ts.default \
    librm_ts_service \
    raydium_selftest \
    rm_test \
    rm_ts_server \
    synaptics_direct_touch_daemon \
    MockNVCP \
    nfc.$(TARGET_BOARD_PLATFORM) \
    nvavp_os_0ff00000.bin \
    nvavp_os_eff00000.bin \
    nvavp_smmu.bin \
    nvavp_vid_ucode_alt.bin \
    nvavp_vid_ucode.bin \
    nvavp_aud_ucode.bin \
    nvblob \
    nvcgcserver \
    NvCPLSvc \
    NvCPLUpdater \
    NvStatService \
    nvcpud \
    powerservice \
    nvdumppublickey \
    nvflash \
    nv_hciattach \
    nvhost \
    nvidia_display \
    nvidl \
    nvsbktool \
    TegraOTA \
    nvsecuretool \
    nvsignblob \
    nvtest \
    overlaymon \
    pbc \
    QuadDSecurityService \
    sensors.tegra \
    shaderfix \
    tegrastats \
    tokenize \
    ttytestapp \
    xaplay \
    tinyplay \
    tinycap \
    tinymix \
    tsechdcp_test \
    keystore.tegra

ifneq ($(filter ap20 t30 t114,$(TARGET_TEGRA_VERSION)),)
    nv_modules += microboot
endif

nv_modules += libtsec_otf_keygen
nv_modules += xusb_sil_rel_fw
nv_modules += libnvtsecmpeg2ts
ifneq ($(filter t114,$(TARGET_TEGRA_VERSION)),)
    nv_modules += nvhost_msenc02.fw
    nv_modules += nvhost_tsec.fw
endif



ifeq ($(BOARD_BUILD_BOOTLOADER),true)
nv_modules += \
    bootloader
endif


ifeq ($(TARGET_TEGRA_VERSION),ap20)
nv_modules += \
    libnvddk_se
endif

ifeq ($(BOARD_HAVE_CONTROLLER_MAPPER),true)
nv_modules += ControllerMapper
endif

# Begin nvsi modules but only on t114+
ifeq ($(filter ap20 t30,$(TARGET_TEGRA_VERSION)),)
nv_nvsi_modules := \
    com.nvidia.nvsi.xml \
    libtsec_wrapper
include $(CLEAR_VARS)
LOCAL_MODULE := nv_nvsi_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_nvsi_modules)
include $(BUILD_PHONY_PACKAGE)
endif
# End nvsi module

# Begin mm modules needed by NV products
nv_mm_modules := \
    libnvomxadaptor \
    libnvaviparserhal \
    libnvasfparserhal \
    libstagefrighthw \
    libaudioservice \
    audio.primary.$(TARGET_BOARD_PLATFORM) \
    audio_policy.$(TARGET_BOARD_PLATFORM) \
    libnvmmlite \
    libnvmmlite_utils \
    libnvmm_manager \
    libnvmm_service \
    libnv_parser \
    libnvmm_writer \
    libnvmm_parser \
    libnvmmcommon \
    libnv3gpwriter \
    libnvbasewriter \
    libnvmm_contentpipe \
    libnvmm_utils \
    libnvmmtransport \
    libnvomx \
    libnvomxilclient \
    libnvavp

include $(CLEAR_VARS)
LOCAL_MODULE := nv_mm_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_mm_modules)
include $(BUILD_PHONY_PACKAGE)
# End mm modules needed by NV products

# Begin codecs modules needed by NV products
nv_codecs_modules := \
    libh264enc \
    libaacdec \
    libnvaacplusenc \
    libmpeg4enc \
    libnvmpeg4dec \
    libnvoggdec \
    libnvwma \
    libnvwmalsl \
    libnvmmlite_audio \
    libnvmmlite_image \
    libnvmmlite_msaudio \
    libnvmmlite_video \
    libnvmm_asfparser \
    libnvbsacdec \
    libnvamrnbcommon \
    libnvamrnbdec \
    libnvamrnbenc \
    libnvamrwbdec \
    libnvamrwbenc \
    libnvmm_vc1_video \
    libnvmm_video \
    libnvaudio_power \
    libnvaudioutils \
    libnvaudio_ratecontrol \
    libnvaudio_memutils \
    libnvilbccommon \
    libnvilbcdec \
    libnvilbcenc \
    libnvvideodec \
    libnvvideoenc \
    libnvsuperjpegdec \
    libh264msenc \
    libvp8msenc \
    libnvmm_aviparser

include $(CLEAR_VARS)
LOCAL_MODULE := nv_codecs_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_codecs_modules)
include $(BUILD_PHONY_PACKAGE)
# End codecs modules needed by NV products

# Begin mjolnir modules needed by NV products
nv_mjolnir_modules := \
    libmjpcap \
    libmjpcapservice_client \
    libmjpcapservice \
    mjpcapservice

include $(CLEAR_VARS)
LOCAL_MODULE := nv_mjolnir_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_mjolnir_modules)
include $(BUILD_PHONY_PACKAGE)
# End mjolnir modules needed by NV products

# Begin tvmr modules needed by NV products
nv_tvmr_modules := \
    libnvtvmr \
    libaudioavp \
    libnvparser

include $(CLEAR_VARS)
LOCAL_MODULE := nv_tvmr_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_tvmr_modules)
include $(BUILD_PHONY_PACKAGE)
# End tvmr modules needed by NV products

# Begin camera modules needed by NV products
nv_camera_modules := \
    libnvcamera \
    libnvcamerahdr \
    libnvcamerautil \
    libnvstitching \
    libnvcam_imageencoder \
    libnvmm_camera \
    libnvdigitalzoom \
    camera.$(TARGET_BOARD_PLATFORM) \
    libnvodm_imager \
    libnvsm

include $(CLEAR_VARS)
LOCAL_MODULE := nv_camera_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_camera_modules)
include $(BUILD_PHONY_PACKAGE)
# End camera modules needed by NV products

# Begin graphics modules needed by NV products
nv_graphics_modules := \
    com.nvidia.graphics \
    libGLESv1_CM_perfhud \
    libGLESv1_CM_tegra \
    libGLESv1_CM_tegra_impl \
    libGLESv2_perfhud \
    libGLESv2_tegra \
    libGLESv2_tegra_impl \
    libEGL_perfhud \
    libEGL_tegra \
    libEGL_tegra_impl \
    gralloc.$(TARGET_BOARD_PLATFORM) \
    hwcomposer.$(TARGET_BOARD_PLATFORM) \
    libnvcms \
    libnvwinsys \
    libnvwsi \
    libnvwsi_core \
    libnvglsi \
    libnvblit \
    libnvddk_2d \
    libnvddk_2d_ap15 \
    libnvddk_2d_ar3d \
    libnvddk_2d_combined \
    libnvddk_2d_fastc \
    libnvddk_2d_swref \
    libnvddk_2d_v2 \
    libardrv_dynamic \
    libnvdrawpath

include $(CLEAR_VARS)
LOCAL_MODULE := nv_graphics_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_graphics_modules)
include $(BUILD_PHONY_PACKAGE)
# End graphics modules needed by NV products

# Begin python modules needed by NV products
nv_python_modules := \
    _collections \
    _fileio \
    _functools \
    _multibytecodec \
    _random \
    _socket \
    _struct \
    _weakref \
    _nvcamera \
    array \
    cmath \
    math \
    strop \
    time \
    datetime \
    operator \
    unicodedata \
    fcntl \
    parser \
    binascii \
    select \
    libpython2.6 \
    python

include $(CLEAR_VARS)
LOCAL_MODULE := nv_python_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_python_modules)
include $(BUILD_PHONY_PACKAGE)
# End python modules needed by NV products

# Begin wfd modules needed by NV products
nv_wfd_modules := \
    nvcap_test \
    libnvcap \
    libnvcapclk \
    libnvcap_video \
    com.nvidia.nvwfd \
    libwfd_common \
    libwfd_sink \
    libwfd_source \
    NvwfdProtocolsPack \
    NvwfdService \
    NvwfdSigmaDutTest \
    libwlbwservice \
    wlbwservice \
    libwlbwjni \
    libjni_nvremote \
    libjni_nvremoteprotopkg \
    libnvremoteevtmgr \
    libnvremotell \
    libnvremoteprotocol

include $(CLEAR_VARS)
LOCAL_MODULE := nv_wfd_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_wfd_modules)
include $(BUILD_PHONY_PACKAGE)
# End wfd modules needed by NV products

# Begin Widevine modules needed by NV products
ifneq ($(filter ap20 t20, $(TARGET_TEGRA_VERSION)),)
    LOCAL_OEMCRYPTO_LEVEL := 1
else
    LOCAL_OEMCRYPTO_LEVEL := 3
endif

nv_wv_modules := \
    test-libwvm \
    test-wvdrmplugin \
    test-wvplayer_L$(LOCAL_OEMCRYPTO_LEVEL) \
    com.google.widevine.software.drm \
    com.google.widevine.software.drm.xml \
    libdrmwvmcommon \
    libwvdrm_L$(LOCAL_OEMCRYPTO_LEVEL) \
    libwvmcommon \
    libwvocs_L$(LOCAL_OEMCRYPTO_LEVEL) \
    libWVStreamControlAPI_L$(LOCAL_OEMCRYPTO_LEVEL) \
    liboemcrypto \
    libwvdrmengine \
    libdrmdecrypt

include $(CLEAR_VARS)
LOCAL_MODULE := nv_wv_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_wv_modules)
include $(BUILD_PHONY_PACKAGE)
# End Widevine modules needed by NV products

# Comment out SECURE_OS_BUILD variable check for now as WAR
# to get default builds passing. tf_daemon needs to be installed
# in system image to get release packages created.
# Begin Trusted Logic modules needed by NV products
#ifeq ($(SECURE_OS_BUILD),y)
nv_tl_modules := \
    libsmapi \
    libtf_crypto_sst \
    tf_daemon
#endif

include $(CLEAR_VARS)
LOCAL_MODULE := nv_tl_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_tl_modules)
include $(BUILD_PHONY_PACKAGE)
# End Trusted Logic modules needed by NV products

# Begin dgpu modules needed by NV products
ifeq ($(NV_MOBILE_DGPU),1)
nv_dgpu_modules := \
    libGLESv1_CM_dgpu_impl \
    libGLESv2_dgpu_impl \
    libglcore \
    mknod \
    nvidia.ko
endif
include $(CLEAR_VARS)
LOCAL_MODULE := nv_dgpu_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_dgpu_modules)
include $(BUILD_PHONY_PACKAGE)
# End dgpu modules needed by NV products

include $(CLEAR_VARS)
LOCAL_MODULE := nvidia_tegra_proprietary_src_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_modules)
LOCAL_REQUIRED_MODULES += $(ALL_NVIDIA_TESTS)
include $(BUILD_PHONY_PACKAGE)
