-include config.mk

include $(IMAGINE_PATH)/make/webos-armv7-gcc.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/webos-armv7
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
