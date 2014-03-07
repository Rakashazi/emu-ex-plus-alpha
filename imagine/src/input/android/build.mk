ifndef inc_input
inc_input := 1

configDefs += CONFIG_INPUT CONFIG_INPUT_ANDROID

SRC += input/android/androidInput.cc

ifneq ($(MACHINE),OUYA)
 configDefs += CONFIG_INPUT_ANDROID_MOGA
 SRC += input/android/moga.cc
endif

endif
