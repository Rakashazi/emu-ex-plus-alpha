include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

ARCH := arm
SUBARCH := armv7
MACHINE := GENERIC_ARMV7
include $(currPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv7s
endif

IOS_FLAGS += -arch armv7s
ASMFLAGS += -arch armv7s
CHOST := $(shell $(CC) -arch armv7s -dumpmachine)
system_externalSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7s
include $(currPath)/iOS-armv7-common.mk
minIOSVer := 6.0