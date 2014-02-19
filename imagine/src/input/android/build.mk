ifndef inc_input
inc_input := 1

configDefs += CONFIG_INPUT CONFIG_INPUT_ANDROID

configDefs += CONFIG_INPUT_ANDROID_MOGA

SRC += input/android/androidInput.cc input/android/moga.cc

endif
