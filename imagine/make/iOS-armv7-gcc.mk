include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

ARCH := arm
SUBARCH := armv7
include $(currPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv7
endif

IOS_FLAGS += -arch armv7
ASMFLAGS += -arch armv7
CHOST := $(shell $(CC) -arch armv7 -dumpmachine)
system_externalSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7
include $(currPath)/iOS-armv7-common.mk
