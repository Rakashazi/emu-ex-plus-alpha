-include config.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/ios-armv7s
objDir := $(buildDir)

include $(IMAGINE_PATH)/make/iOS-armv7s-gcc.mk

installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk

