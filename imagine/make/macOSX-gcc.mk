ENV := macosx

compiler_noSanitizeAddress := 1
config_compiler := clang
AR := ar
ifeq ($(origin CC), default)
 CC := clang
endif
include $(buildSysPath)/clang.mk

ifdef RELEASE
 COMPILE_FLAGS += -DNS_BLOCK_ASSERTIONS
endif

XCODE_PATH := $(shell xcode-select --print-path)
OSX_SYSROOT ?= $(XCODE_PATH)/Platforms//MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk
OSX_FLAGS = -isysroot $(OSX_SYSROOT) -mmacosx-version-min=10.7
CPPFLAGS += $(OSX_FLAGS)
LDFLAGS += $(OSX_FLAGS)

LDFLAGS += -dead_strip -Wl,-S,-x
WHOLE_PROGRAM_CFLAGS := -fipa-pta -fwhole-program

extraSysroot := /opt/local
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
CPPFLAGS += -I$(extraSysroot)/include

include $(buildSysPath)/package/stdc++.mk