include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(buildSysPath)/linux-gcc.mk

CHOST := $(shell $(CC) -dumpmachine)
ARCH := x86
COMPILE_FLAGS += -m32 -march=pentium4 -mtune=generic
LDFLAGS += -m32
ASMFLAGS += -m32

ifdef extraSysroot
 PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
 PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
 PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
 CPPFLAGS += -I$(extraSysroot)/include
 LDLIBS += -L$(extraSysroot)/lib
endif

ifneq ($(filter x86_64-%,$(CHOST)),)
 x86PkgConfigPath ?= /usr/lib32/pkgconfig
endif

PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(x86PkgConfigPath)