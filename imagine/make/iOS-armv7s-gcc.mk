include $(IMAGINE_PATH)/make/config.mk

ARCH := arm
SUBARCH := armv7
MACHINE := GENERIC_ARMV7
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv7s
endif

IOS_FLAGS += -arch armv7s
ASMFLAGS += -arch armv7s
COMPILE_FLAGS += -mdynamic-no-pic
CHOST := $(shell $(CC) -arch armv7s -dumpmachine)

extraSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7s
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
CPPFLAGS += -I$(extraSysroot)/include

include $(buildSysPath)/iOS-armv7-common.mk
minIOSVer = 6.0