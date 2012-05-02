include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv6
endif

CHOST := arm-apple-darwin10
IOS_FLAGS += -arch armv6
ASMFLAGS += -arch armv6
system_externalSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv6
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib