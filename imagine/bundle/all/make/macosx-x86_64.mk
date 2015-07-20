-include config.mk

RELEASE := 1
include $(IMAGINE_PATH)/make/macOSX-x86_64-gcc.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/macosx-x86_64
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk

