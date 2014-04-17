ifndef inc_base
inc_base := 1

include $(IMAGINE_PATH)/make/package/egl.mk

configDefs += CONFIG_BASE_ANDROID

# TODO: android_minSDK should only apply to APK metadata
ifdef android_minLibSDK
 CPPFLAGS += -DCONFIG_ENV_ANDROID_MINSDK=$(android_minLibSDK)
 configDefs += CONFIG_ENV_ANDROID_MINSDK=$(android_minLibSDK)
else
 CPPFLAGS += -DCONFIG_ENV_ANDROID_MINSDK=$(android_minSDK)
 configDefs += CONFIG_ENV_ANDROID_MINSDK=$(android_minSDK)
endif

ifeq ($(android_hasSDK9), 1)
 SRC += base/common/Base.cc base/android/androidBase.cc base/android/AndroidWindow.cc \
  base/android/ALooperEventLoop.cc base/common/timer/TimerFD.cc base/common/PosixPipe.cc
 LDLIBS += -landroid
else
 #SRC += base/android/main.cc
endif

SRC += base/android/privateApi/libhardware.c

endif
