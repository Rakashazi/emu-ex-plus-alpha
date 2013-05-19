include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

ARCH := arm
SUBARCH := armv6
MACHINE := GENERIC_ARMV6
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv6
endif

IOS_SYSROOT = /Applications/Xcode44.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.1.sdk
IOS_FLAGS += -arch armv6
ASMFLAGS += -arch armv6
CHOST := $(shell $(CC) -arch armv6 -dumpmachine)

extraSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv6
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(extraSysroot)
CPPFLAGS += -I$(extraSysroot)/include
LDLIBS += -L$(extraSysroot)/lib
