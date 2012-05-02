config_ios_jb := 1
O_RELEASE := 1
O_LTO := 1
#config_compiler := gcc
#O_SIZE := 1
include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk
include build.mk
