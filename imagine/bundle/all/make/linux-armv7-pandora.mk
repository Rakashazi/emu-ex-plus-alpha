-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/linux-armv7-pandora
installDir := $(IMAGINE_PATH)/bundle/linux-armv7-pandora

include $(IMAGINE_PATH)/make/linux-armv7-pandora-gcc.mk

include common.mk

