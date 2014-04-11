-include config.mk

compiler_noSanitizeAddress := 1
include $(IMAGINE_PATH)/make/win32-x86.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/win32-x86
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
