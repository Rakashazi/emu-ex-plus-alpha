-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/android-x86
installDir := $(IMAGINE_PATH)/bundle/android/x86

include $(IMAGINE_PATH)/make/android-x86-gcc.mk

include common.mk
