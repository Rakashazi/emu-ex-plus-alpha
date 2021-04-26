ENV := ios
ENV_KERNEL := mach
CROSS_COMPILE := 1

ifndef target
 target = $(metadata_exec)
endif

ifndef targetDir
 ifdef O_RELEASE
  targetDir := target/iOS/bin-release
 else
  targetDir := target/iOS/bin-debug
 endif
endif

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

ifdef CCTOOLS_TOOCHAIN_PATH
AR := $(firstword $(wildcard $(CCTOOLS_TOOCHAIN_PATH)/bin/*-ar))
CC := $(firstword $(wildcard $(CCTOOLS_TOOCHAIN_PATH)/bin/*-clang))
CXX := $(firstword $(wildcard $(CCTOOLS_TOOCHAIN_PATH)/bin/*-clang++))
LD := $(CXX)
iosSimulatorSDKsPath := $(CCTOOLS_TOOCHAIN_PATH)/SDK
iosSDKsPath := $(CCTOOLS_TOOCHAIN_PATH)/SDK
CPPFLAGS += -I$(firstword $(wildcard $(iosSDKsPath)/iPhoneOS*.sdk/usr/include/c++))
LDFLAGS_SYSTEM += -Wl,-force_load,$(firstword $(wildcard $(iosSDKsPath)/iPhoneOS*.sdk/usr/lib/arc/libarclite_iphoneos.a))
else
XCODE_PATH := $(shell xcode-select --print-path)
iosSimulatorSDKsPath := $(XCODE_PATH)/Platforms/iPhoneSimulator.platform/Developer/SDKs
iosSDKsPath := $(XCODE_PATH)/Platforms/iPhoneOS.platform/Developer/SDKs
LDFLAGS_SYSTEM += -fobjc-arc
endif

ifeq ($(ARCH),x86)
 ifndef IOS_SYSROOT
  ifdef IOS_SDK
   IOS_SYSROOT := $(iosSimulatorSDKsPath)/iPhoneSimulator$(IOS_SDK).sdk
  else
   IOS_SYSROOT := $(firstword $(wildcard $(iosSimulatorSDKsPath)/iPhoneSimulator*.sdk))
  endif
 endif
 IOS_FLAGS = -isysroot $(IOS_SYSROOT) -mios-simulator-version-min=$(minIOSVer)
 OBJCFLAGS += -fobjc-abi-version=2 -fobjc-legacy-dispatch
else
 ifndef IOS_SYSROOT
  ifdef IOS_SDK
   IOS_SYSROOT := $(iosSDKsPath)/iPhoneOS$(IOS_SDK).sdk
  else
   IOS_SYSROOT := $(firstword $(wildcard $(iosSDKsPath)/iPhoneOS*.sdk))
  endif
 endif
 IOS_FLAGS = -isysroot $(IOS_SYSROOT) -miphoneos-version-min=$(minIOSVer)
endif
CPPFLAGS += $(IOS_FLAGS)
LDFLAGS_SYSTEM += $(IOS_FLAGS)

ifeq ($(SUBARCH),armv6)
 ifdef iosNoDeadStripArmv6
  ios_noDeadStrip := 1
 endif
endif
ifndef ios_noDeadStrip
 LDFLAGS_SYSTEM += -dead_strip
endif
ifdef RELEASE
 LDFLAGS_SYSTEM += -Wl,-S,-x,-dead_strip_dylibs
else
 LDFLAGS_SYSTEM += -Wl,-x,-dead_strip_dylibs
endif
LDFLAGS += -Wl,-no_pie

# clang SVN doesn't seem to handle ASM properly so use as directly
AS := as
ASMFLAGS :=
