ifndef inc_base
inc_base := 1

configDefs += CONFIG_BASE_ANDROID

CPPFLAGS += -DCONFIG_ENV_ANDROID_MINSDK=$(android_minSDK)

ifeq ($(android_hasSDK9), 1)
 SRC += base/android/mainNative.cc base/android/nativeGlue.cc
 LDLIBS += -lEGL -landroid
else
 SRC += base/android/main.cc
endif

endif
