-include config.mk

include $(IMAGINE_PATH)/make/ps3-gcc.mk

tempDir = /tmp/imagine-bundle/$(pkgName)
buildDir = $(tempDir)/build/ps3
installDir = $(IMAGINE_SDK_PATH)/$(IMAGINE_SDK_PLATFORM)

include common.mk
