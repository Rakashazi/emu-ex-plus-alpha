NO_SRC_DEPS := 1

buildDir := build/ios-armv7
objDir := $(buildDir)/obj
installDir := ../../../darwin-iOS/armv7
installDirUniversal := ../../../darwin-iOS

include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk

include common.mk

