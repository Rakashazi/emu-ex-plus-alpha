-include config.mk

RELEASE := 1
tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/linux-x86
buildPath = $(buildDir)
include $(IMAGINE_PATH)/make/linux-x86-gcc.mk

installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk

