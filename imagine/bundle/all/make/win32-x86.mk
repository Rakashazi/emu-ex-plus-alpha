-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/win32-x86
installDir := $(IMAGINE_PATH)/bundle/win32-x86

compiler_noSanitizeAddress := 1
include $(IMAGINE_PATH)/make/win32-x86.mk

include common.mk

