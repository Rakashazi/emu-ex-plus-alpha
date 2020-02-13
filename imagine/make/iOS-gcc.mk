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
LDFLAGS_SYSTEM += -fobjc-arc

XCODE_PATH := $(shell xcode-select --print-path)
ifeq ($(ARCH),x86)
 iosSDKsPath := $(XCODE_PATH)/Platforms/iPhoneSimulator.platform/Developer/SDKs
 ifndef IOS_SYSROOT
  ifdef IOS_SDK
   IOS_SYSROOT := $(iosSDKsPath)/iPhoneSimulator$(IOS_SDK).sdk
  else
   IOS_SYSROOT := $(firstword $(wildcard $(iosSDKsPath)/iPhoneSimulator*.sdk))
  endif
 endif
 IOS_FLAGS = -isysroot $(IOS_SYSROOT) -mios-simulator-version-min=$(minIOSVer)
 OBJCFLAGS += -fobjc-abi-version=2 -fobjc-legacy-dispatch
else
 iosSDKsPath := $(XCODE_PATH)/Platforms/iPhoneOS.platform/Developer/SDKs
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
