include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv7
endif

ifndef ios_armv7State
 ios_armv7State := -mthumb
endif

# ARMv7 with LTO needs at least iOS 4.3
minIOSVer := 4.3
CHOST := arm-apple-darwin10
IOS_FLAGS += -arch armv7 $(ios_armv7State)
ASMFLAGS += -arch armv7
system_externalSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib