-include config.mk

compiler_noSanitizeAddress := 1
include $(IMAGINE_PATH)/make/linux-x86-gcc.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/linux-x86
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)-static

include common.mk

