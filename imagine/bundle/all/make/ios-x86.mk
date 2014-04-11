-include config.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/ios-x86
objDir := $(buildDir)

include $(IMAGINE_PATH)/make/iOS-x86-gcc.mk

installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
