ENV := macosx
ENV_KERNEL := mach

config_compiler := clang
AR := ar
ifeq ($(origin CC), default)
 CC := clang
 CXX := clang++
endif
include $(buildSysPath)/clang.mk

linkLoadableModuleAction = -bundle -flat_namespace -undefined suppress

ifdef RELEASE
 CPPFLAGS += -DNS_BLOCK_ASSERTIONS
endif

OBJCFLAGS += -fobjc-arc
LDFLAGS_SYSTEM += -fobjc-arc

XCODE_PATH := $(shell xcode-select --print-path)
macosSDKsPath := $(XCODE_PATH)/Platforms/MacOSX.platform/Developer/SDKs
ifndef OSX_SYSROOT
 ifdef OSX_SDK
  OSX_SYSROOT := $(macosSDKsPath)/MacOSX$(IOS_SDK).sdk
 else
  OSX_SYSROOT := $(firstword $(wildcard $(macosSDKsPath)/MacOSX*.sdk))
 endif
endif
OSX_FLAGS = -isysroot $(OSX_SYSROOT) -mmacosx-version-min=10.8
CPPFLAGS += $(OSX_FLAGS)
LDFLAGS_SYSTEM += $(OSX_FLAGS)

ifdef RELEASE
 LDFLAGS_SYSTEM += -dead_strip -Wl,-S,-x,-dead_strip_dylibs
else
 LDFLAGS_SYSTEM += -dead_strip -Wl,-x,-dead_strip_dylibs
endif

macportsPath := /opt/local
macportsPkgconfigPath := $(macportsPath)/lib/pkgconfig
