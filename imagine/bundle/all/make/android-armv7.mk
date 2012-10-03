-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/android-armv7
installDir := $(IMAGINE_PATH)/bundle/android/armv7

include $(IMAGINE_PATH)/make/android-armv7-gcc.mk

include common.mk
