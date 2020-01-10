include $(IMAGINE_PATH)/make/config.mk

ARCH := arm
SUBARCH := armv7
minIOSVer ?= 6.0
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv7s
endif

IOS_FLAGS += -arch armv7s
ASMFLAGS += -arch armv7s
CFLAGS_CODEGEN += -mdynamic-no-pic -faligned-allocation
CHOST := $(shell $(CC) -arch armv7s -dumpmachine)

include $(buildSysPath)/iOS-armv7-common.mk