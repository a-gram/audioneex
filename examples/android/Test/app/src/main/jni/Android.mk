LOCAL_PATH := $(call my-dir)

TARGET_TOOLCHAIN := $(NDK_TOOLCHAIN_VERSION)

LIB_DIR_TAG := android-$(TARGET_ARCH_ABI)-$(TARGET_TOOLCHAIN)


###########################################################
#                      User Config                        #
###########################################################


MY_INCLUDES=$(LOCAL_PATH)/include \
            $(LOCAL_PATH)/../../../../../../../src \
            $(LOCAL_PATH)/../../../../../../../audio \
            $(LOCAL_PATH)/../../../../../../../DAO \
            /usr/local/include \
            /usr/local/include/tcabinet \
            /usr/local/share/boost_1_66_0

MY_AUDIONEEX_LIB_DIR := $(LOCAL_PATH)/../../../../../../../lib/$(LIB_DIR_TAG)/$(APP_OPTIM)
MY_DATASTORE_LIB_DIR := $(LOCAL_PATH)/../../../../../../../lib/$(LIB_DIR_TAG)/$(APP_OPTIM)


###########################################################
#                      Build Config                       #
###########################################################


include $(CLEAR_VARS)
LOCAL_MODULE := audioneex
LOCAL_SRC_FILES := $(MY_AUDIONEEX_LIB_DIR)/libaudioneex.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tokyocabinet
LOCAL_SRC_FILES := $(MY_DATASTORE_LIB_DIR)/libtokyocabinet.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tcDAO
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../../../../DAO/TCDataStore.cpp
LOCAL_C_INCLUDES := $(MY_INCLUDES)
LOCAL_CPPFLAGS += -std=c++11
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := audioneex-test
LOCAL_SRC_FILES := audioneex-test.cpp
LOCAL_C_INCLUDES += $(MY_INCLUDES)
LOCAL_CPPFLAGS += -std=c++11
LOCAL_STATIC_LIBRARIES := tcDAO
LOCAL_SHARED_LIBRARIES := audioneex tokyocabinet
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
