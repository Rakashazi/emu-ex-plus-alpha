-include config.mk

include $(IMAGINE_PATH)/make/linux-armv7-pandora-gcc.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/linux-armv7-pandora
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk

