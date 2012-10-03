-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/android-armv6
installDir := $(IMAGINE_PATH)/bundle/android/armv6
android_minSDK := 5

include $(IMAGINE_PATH)/make/android-armv6-gcc.mk

include common.mk
