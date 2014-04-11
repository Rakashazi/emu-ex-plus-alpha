-include config.mk

include $(IMAGINE_PATH)/make/android-armv6-gcc.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/android-armv6
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
