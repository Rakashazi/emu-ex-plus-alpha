NO_SRC_DEPS := 1

buildDir := build/ios-armv6
objDir := $(buildDir)/obj
installDir := ../../../darwin-iOS/armv6
installDirUniversal := ../../../darwin-iOS

include $(IMAGINE_PATH)/make/iOS-armv6-gcc.mk

include common.mk

