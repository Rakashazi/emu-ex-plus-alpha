include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

ARCH := x86
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -x86
endif

minIOSVer = 5.0
IOS_FLAGS += -arch i386
ASMFLAGS += -arch i386
CHOST := $(shell $(CC) -arch i386 -dumpmachine)

extraSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/x86
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(extraSysroot)
CPPFLAGS += -I$(extraSysroot)/include
LDLIBS += -L$(extraSysroot)/lib