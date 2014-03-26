-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/macosx-x86_64
installDir := $(IMAGINE_PATH)/bundle/macosx/x86_64

compiler_noSanitizeAddress := 1
include $(IMAGINE_PATH)/make/macOSX-x86_64-gcc.mk

include common.mk

