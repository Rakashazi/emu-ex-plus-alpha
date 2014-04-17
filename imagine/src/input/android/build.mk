ifndef inc_input_android
inc_input_android := 1

include $(imagineSrcDir)/input/build.mk

configDefs += CONFIG_INPUT_ANDROID

SRC += input/android/androidInput.cc

ifneq ($(MACHINE),OUYA)
 configDefs += CONFIG_INPUT_ANDROID_MOGA
 SRC += input/android/moga.cc
endif

endif
