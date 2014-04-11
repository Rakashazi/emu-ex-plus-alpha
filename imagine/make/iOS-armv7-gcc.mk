include $(IMAGINE_PATH)/make/config.mk

ARCH := arm
SUBARCH := armv7
MACHINE := GENERIC_ARMV7
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv7
endif

IOS_FLAGS += -arch armv7
ASMFLAGS += -arch armv7
COMPILE_FLAGS += -mdynamic-no-pic
CHOST := $(shell $(CC) -arch armv7 -dumpmachine)

include $(buildSysPath)/iOS-armv7-common.mk
