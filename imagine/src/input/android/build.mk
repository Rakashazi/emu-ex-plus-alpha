ifndef inc_input
inc_input := 1

configDefs += CONFIG_INPUT CONFIG_INPUT_ANDROID

ifeq ($(android_hasSDK9), 1)
 SRC += input/android/androidNative.cc
else
 SRC += input/android/android.cc
endif

endif
