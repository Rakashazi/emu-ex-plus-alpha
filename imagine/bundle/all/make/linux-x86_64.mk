-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/linux-x86_64
installDir := $(IMAGINE_PATH)/bundle/linux-x86_64

compiler_noSanitizeAddress := 1
include $(IMAGINE_PATH)/make/linux-x86_64-gcc.mk

include common.mk

