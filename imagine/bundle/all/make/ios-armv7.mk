-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/ios-armv7
installDir := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7
installDirUniversal := $(IMAGINE_PATH)/bundle/darwin-iOS
objDir := $(buildDir)

include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk

include common.mk

