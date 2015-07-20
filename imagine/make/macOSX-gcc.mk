ENV := macosx

compiler_noSanitizeAddress := 1
config_compiler := clang
AR := ar
ifeq ($(origin CC), default)
 CC := clang
 CXX := clang++
endif
include $(buildSysPath)/clang.mk

ifdef RELEASE
 CPPFLAGS += -DNS_BLOCK_ASSERTIONS
endif

OBJCFLAGS += -fobjc-arc
LDFLAGS += -fobjc-arc

XCODE_PATH := $(shell xcode-select --print-path)
OSX_SYSROOT ?= $(XCODE_PATH)/Platforms//MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk
OSX_FLAGS = -isysroot $(OSX_SYSROOT) -mmacosx-version-min=10.8
CPPFLAGS += $(OSX_FLAGS)
LDFLAGS += $(OSX_FLAGS)

ifdef RELEASE
 LDFLAGS += -dead_strip -Wl,-S,-x,-dead_strip_dylibs
else
 LDFLAGS += -dead_strip -Wl,-x,-dead_strip_dylibs
endif

macportsPath := /opt/local
macportsPkgconfigPath := $(macportsPath)/lib/pkgconfig
CPPFLAGS += -I$(macportsPath)/include
