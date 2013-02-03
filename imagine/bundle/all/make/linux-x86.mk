-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/linux-x86
installDir := $(IMAGINE_PATH)/bundle/linux-x86

include $(IMAGINE_PATH)/make/linux-x86-gcc.mk

include common.mk

