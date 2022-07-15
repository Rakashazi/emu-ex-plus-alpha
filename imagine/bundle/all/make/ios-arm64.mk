-include config.mk

RELEASE := 1
tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/ios-arm64
buildPath = $(buildDir)
include $(IMAGINE_PATH)/make/ios-arm64.mk

installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
