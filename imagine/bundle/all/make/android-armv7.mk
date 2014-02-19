-include config.mk

# don't LTO with -marm since oupt will eventually be combined with THUMB code
ifeq ($(android_armv7State),-marm)
undefine O_LTO
endif

buildDir = /tmp/imagine-bundle/$(pkgName)/build/android-armv7
installDir := $(IMAGINE_PATH)/bundle/android/armv7

include $(IMAGINE_PATH)/make/android-armv7-gcc.mk

include common.mk
