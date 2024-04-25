-include config.mk

# don't LTO with -marm since oupt will eventually be combined with THUMB code
ifeq ($(ios_armv7State),-marm)
 LTO_MODE := off
endif

RELEASE := 1
tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/ios-armv7
buildPath = $(buildDir)
include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk

installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
