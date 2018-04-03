
LOCAL_PATH := $(call my-dir)


##########################################################
#                      User Config                       #
##########################################################

# Adjust these library paths according to your environment.

MY_LOCAL_INCLUDE := /usr/local/include \
                    /usr/local/share/boost_1_66_0

# Android libs are supposed to be in a dir layout as follows
# /my/android/lib/<arch>
MY_ANDROID_LIBS := /opt/android/lib/$(TARGET_ARCH_ABI)


##########################################################
#                    Modules Config                      #
##########################################################


AX_INCLUDES := $(MY_LOCAL_INCLUDE) \
               src \
               src/ident \
               src/index \
               src/quant \
               src/audiocodes \
               audio \
               tools
               
AX_SOURCES := \
    src/ident/Fingerprint.cpp \
    src/ident/Matcher.cpp \
    src/ident/MatchFuzzyClassifier.cpp \
    src/ident/Recognizer.cpp \
    src/index/BlockCodec.cpp \
    src/index/Indexer.cpp \
    src/quant/Codebook.cpp \
    src/audiocodes/AudioCodes.cpp

# Audioneex audio codes includes & co.

#AX_CODES_INCLUDE := audiocodes
#AX_CODES_SOURCES := audiocodes/AudioCodes.cpp

# Audioneex audio codes module

#include $(CLEAR_VARS)
#LOCAL_MODULE := axcodes
#LOCAL_SRC_FILES := $(AX_CODES_SOURCES)
#LOCAL_C_INCLUDES := $(AX_CODES_INCLUDE)
#LOCAL_CPPFLAGS += -std=c++11 -fexceptions -fvisibility=hidden -fno-rtti
#include $(BUILD_STATIC_LIBRARY)

ifdef AUDIONEEX_DLL
# SHARED LIB

# FFTSS module

include $(CLEAR_VARS)
LOCAL_MODULE := fftss
LOCAL_SRC_FILES := $(MY_ANDROID_LIBS)/libfftss.a
include $(PREBUILT_STATIC_LIBRARY)

# Audioneex shared lib module

include $(CLEAR_VARS)
LOCAL_MODULE := audioneex
LOCAL_SRC_FILES := $(AX_SOURCES)
LOCAL_C_INCLUDES := $(AX_INCLUDES)
LOCAL_CPPFLAGS += -DAUDIONEEX_API_EXPORT -DAUDIONEEX_DLL
LOCAL_CPPFLAGS += -std=c++11 -fexceptions -fvisibility=hidden -fno-rtti
LOCAL_STATIC_LIBRARIES := fftss
include $(BUILD_SHARED_LIBRARY)

else
# STATIC LIB

# Audioneex static lib module

include $(CLEAR_VARS)
LOCAL_MODULE := audioneex
LOCAL_SRC_FILES := $(AX_SOURCES)
LOCAL_C_INCLUDES := $(AX_INCLUDES)
LOCAL_CPPFLAGS += -std=c++11 -fexceptions
include $(BUILD_STATIC_LIBRARY)

endif
