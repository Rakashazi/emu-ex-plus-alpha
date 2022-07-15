-include config.mk

RELEASE := 1
tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/linux-armv7-pandora
buildPath = $(buildDir)
include $(IMAGINE_PATH)/make/linux-armv7-pandora-gcc.mk

installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk

