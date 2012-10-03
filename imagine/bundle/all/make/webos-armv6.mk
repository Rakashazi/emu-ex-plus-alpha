-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/webos-armv6
installDir := $(IMAGINE_PATH)/bundle/webos/armv6

include $(IMAGINE_PATH)/make/webos-armv6-gcc.mk

include common.mk
