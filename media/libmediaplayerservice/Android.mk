LOCAL_PATH:= $(call my-dir)

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

# RESOURCE MANAGER
ifeq ($(strip $(BOARD_USES_RESOURCE_MANAGER)),true)
LOCAL_CFLAGS += -DRESOURCE_MANAGER
endif
# RESOURCE MANAGER

LOCAL_SRC_FILES:=               \
    ActivityManager.cpp         \
    Crypto.cpp                  \
    Drm.cpp                     \
    HDCP.cpp                    \
    MediaPlayerFactory.cpp      \
    MediaPlayerService.cpp      \
    MediaRecorderClient.cpp     \
    MetadataRetrieverClient.cpp \
    MidiFile.cpp                \
    MidiMetadataRetriever.cpp   \
    RemoteDisplay.cpp           \
    SharedLibrary.cpp           \
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp     \
    TestPlayerStub.cpp          \

ifeq ($(BUILD_WITH_AMLOGIC_PLAYER),true)
    LOCAL_SRC_FILES +=                          \
        AmSuperPlayer.cpp                       \
        AmlogicPlayer.cpp                       \
        SubSource.cpp                       \
        AmlogicPlayerRender.cpp                 \
        AmlogicPlayerStreamSource.cpp           \
        AmlogicPlayerStreamSourceListener.cpp   \
        AmlogicPlayerExtractorDemux.cpp         \
        AmlogicPlayerExtractorDataSource.cpp    \
        AmlogicPlayerDataSouceProtocol.cpp      \
        AmlPlayerMetadataRetriever.cpp
endif

LOCAL_SHARED_LIBRARIES :=       \
    libbinder                   \
    libcamera_client            \
    libcutils                   \
    liblog                      \
    libdl                       \
    libgui                      \
    libmedia                    \
    libsonivox                  \
    libstagefright              \
    libstagefright_foundation   \
    libstagefright_omx          \
    libstagefright_wfd          \
    libutils                    \
    libdl                       \
    libvorbisidec               \
    libdrmframework		\

LOCAL_STATIC_LIBRARIES :=       \
    libstagefright_nuplayer     \
    libstagefright_rtsp         \
    libmedia_helper             \

LOCAL_C_INCLUDES :=                                                 \
    $(call include-path-for, graphics corecg)                       \
    $(TOP)/frameworks/av/media/libstagefright/include               \
    $(TOP)/frameworks/av/media/libstagefright/rtsp                  \
    $(TOP)/frameworks/av/media/libstagefright/wifi-display          \
    $(TOP)/frameworks/native/include/media/openmax                  \
    $(TOP)/external/tremolo/Tremolo                                 \

ifeq ($(BOARD_USES_QCOM_HARDWARE),true)
    ifneq ($(TARGET_QCOM_MEDIA_VARIANT),)
        LOCAL_C_INCLUDES += \
            $(TOP)/hardware/qcom/media-$(TARGET_QCOM_MEDIA_VARIANT)/mm-core/inc
    else
        LOCAL_C_INCLUDES += \
            $(TOP)/hardware/qcom/media/mm-core/inc
    endif
endif

ifeq ($(BUILD_WITH_AMLOGIC_PLAYER),true)
AMPLAYER_APK_DIR=$(TOP)/device/ainol/elf2/packages/LibPlayer/
LOCAL_C_INCLUDES +=\
        $(AMPLAYER_APK_DIR)/amplayer/player/include     \
        $(AMPLAYER_APK_DIR)/amplayer/control/include    \
        $(AMPLAYER_APK_DIR)/amadec/include      \
        $(AMPLAYER_APK_DIR)/amcodec/include     \
        $(AMPLAYER_APK_DIR)/amavutils/include     \
	$(AMPLAYER_APK_DIR)/amvdec/include           \
        $(AMPLAYER_APK_DIR)/amffmpeg/ \
	$(TOP)/device/ainol/elf2/include

LOCAL_SHARED_LIBRARIES += libui
LOCAL_SHARED_LIBRARIES +=libamplayer libamavutils libamvdec
LOCAL_CFLAGS += -DBUILD_WITH_AMLOGIC_PLAYER=1
endif

LOCAL_MODULE:= libmediaplayerservice

ifeq ($(TARGET_ENABLE_QC_AV_ENHANCEMENTS),true)
    LOCAL_CFLAGS += -DENABLE_QC_AV_ENHANCEMENTS
    LOCAL_C_INCLUDES += $(TOP)/frameworks/av/include/media
    ifneq ($(TARGET_QCOM_MEDIA_VARIANT),)
        LOCAL_C_INCLUDES += \
            $(TOP)/hardware/qcom/media-$(TARGET_QCOM_MEDIA_VARIANT)/mm-core/inc
    else
        LOCAL_C_INCLUDES += \
            $(TOP)/hardware/qcom/media/mm-core/inc
    endif
endif #TARGET_ENABLE_QC_AV_ENHANCEMENTS

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
