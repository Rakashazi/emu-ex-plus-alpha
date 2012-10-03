-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/ios-armv6
installDir := $(IMAGINE_PATH)/bundle/darwin-iOS/armv6
installDirUniversal := $(IMAGINE_PATH)/bundle/darwin-iOS
objDir := $(buildDir)

include $(IMAGINE_PATH)/make/iOS-armv6-gcc.mk

include common.mk

