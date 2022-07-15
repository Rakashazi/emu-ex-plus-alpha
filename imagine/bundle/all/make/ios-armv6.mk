-include config.mk

RELEASE := 1
tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/ios-armv6
buildPath = $(buildDir)
include $(IMAGINE_PATH)/make/iOS-armv6-gcc.mk

installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
