-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/ps3
installDir := $(IMAGINE_PATH)/bundle/ps3

include $(IMAGINE_PATH)/make/ps3-gcc.mk

include common.mk
