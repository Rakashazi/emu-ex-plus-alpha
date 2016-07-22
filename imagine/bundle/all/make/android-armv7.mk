-include config.mk

# don't LTO with -marm since output will eventually be combined with THUMB code
ifeq ($(android_armv7State),-marm)
 LTO_MODE := off
endif

RELEASE := 1
include $(IMAGINE_PATH)/make/android-armv7-gcc.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/android-armv7
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
