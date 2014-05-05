ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk

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

SRC += base/android/androidBase.cc base/android/AndroidWindow.cc \
 base/android/AndroidScreen.cc base/android/ALooperEventLoop.cc \
 base/common/timer/TimerFD.cc base/common/PosixPipe.cc \
 base/android/privateApi/libhardware.c
LDLIBS += -landroid

endif
