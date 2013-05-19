include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../config.mk
ios_makefileOpts ?= O_RELEASE=1 O_LTO=1
ios_imagineLibPathARMv6 ?= $(IMAGINE_PATH)/lib/ios-armv6-release
ios_imagineIncludePathARMv6 ?= $(IMAGINE_PATH)/build/ios-armv6-release/gen
ios_imagineLibPathARMv7 ?= $(IMAGINE_PATH)/lib/ios-armv7-release
ios_imagineIncludePathARMv7 ?= $(IMAGINE_PATH)/build/ios-armv7-release/gen
include $(buildSysPath)/shortcut/meta-builds/ios.mk