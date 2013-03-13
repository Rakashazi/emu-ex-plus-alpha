-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/ios-armv7s
installDir := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7s
installDirUniversal := $(IMAGINE_PATH)/bundle/darwin-iOS
objDir := $(buildDir)

include $(IMAGINE_PATH)/make/iOS-armv7s-gcc.mk

include common.mk

