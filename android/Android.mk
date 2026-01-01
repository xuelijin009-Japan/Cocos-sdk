LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := videocall_plugin
LOCAL_SRC_FILES := \
    ../../src/VideoCallPlugin.cpp \
    ../../src/core/VideoCallCore.cpp \
    jni/VideoCallJNI.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../include \
    $(LOCAL_PATH)/../../src

LOCAL_CPPFLAGS += -std=c++17 -DVISCOCALL_ANDROID

include $(BUILD_SHARED_LIBRARY)
