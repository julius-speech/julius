LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE           := PortAudio
LOCAL_SRC_FILES        := src/src/hostapi/opensles/pa_android_opensles.c
LOCAL_STATIC_LIBRARIES := androidSL
LOCAL_C_INCLUDES       := $(LOCAL_PATH)/include \
                          $(LOCAL_PATH)/src/src/common

include $(BUILD_STATIC_LIBRARY)
