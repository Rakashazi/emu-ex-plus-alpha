-include config.mk

# don't LTO with -marm since oupt will eventually be combined with THUMB code
ifeq ($(ios_armv7State),-marm)
 O_LTO :=
endif

buildDir = /tmp/imagine-bundle/$(pkgName)/build/ios-armv7
installDir := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7
installDirUniversal := $(IMAGINE_PATH)/bundle/darwin-iOS
objDir := $(buildDir)

include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk

include common.mk
