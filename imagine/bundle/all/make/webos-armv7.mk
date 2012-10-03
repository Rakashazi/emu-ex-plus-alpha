-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/webos-armv7
installDir := $(IMAGINE_PATH)/bundle/webos/armv7

include $(IMAGINE_PATH)/make/webos-armv7-gcc.mk

include common.mk
