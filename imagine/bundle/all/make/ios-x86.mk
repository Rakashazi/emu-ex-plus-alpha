-include config.mk

buildDir = /tmp/imagine-bundle/$(pkgName)/build/ios-x86
installDir := $(IMAGINE_PATH)/bundle/darwin-iOS/x86
installDirUniversal := $(IMAGINE_PATH)/bundle/darwin-iOS
objDir := $(buildDir)

include $(IMAGINE_PATH)/make/iOS-x86-gcc.mk

include common.mk

