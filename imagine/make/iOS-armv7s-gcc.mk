include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

ARCH := arm
SUBARCH := armv7
MACHINE := GENERIC_ARMV7
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv7s
endif

IOS_FLAGS += -arch armv7s
ASMFLAGS += -arch armv7s
CHOST := $(shell $(CC) -arch armv7s -dumpmachine)

extraSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7s
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(extraSysroot)
CPPFLAGS += -I$(extraSysroot)/include
LDLIBS += -L$(extraSysroot)/lib

include $(buildSysPath)/iOS-armv7-common.mk
minIOSVer = 6.0