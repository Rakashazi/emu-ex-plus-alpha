ENV := ios
CROSS_COMPILE := 1
configDefs += CONFIG_MACHINE_$(MACHINE)
binStatic := 1

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

OBJCFLAGS += -fobjc-arc
LDFLAGS += -fobjc-arc

 # base engine code needs at least iOS 4.0
minIOSVer = 4.0
IOS_SDK ?= 7.1
XCODE_PATH := $(shell xcode-select --print-path)
ifeq ($(ARCH),x86)
 IOS_SYSROOT ?= $(XCODE_PATH)/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator$(IOS_SDK).sdk
 IOS_FLAGS = -isysroot $(IOS_SYSROOT) -mios-simulator-version-min=$(minIOSVer)
 OBJCFLAGS += -fobjc-abi-version=2 -fobjc-legacy-dispatch
else
 IOS_SYSROOT ?= $(XCODE_PATH)/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS$(IOS_SDK).sdk
 IOS_FLAGS = -isysroot $(IOS_SYSROOT) -miphoneos-version-min=$(minIOSVer)
endif
CPPFLAGS += $(IOS_FLAGS)
LDFLAGS += $(IOS_FLAGS)

ifeq ($(SUBARCH),armv6)
 ifdef iosNoDeadStripArmv6
  ios_noDeadStrip := 1
 endif
endif
ifndef ios_noDeadStrip
 LDFLAGS += -dead_strip
endif
ifdef RELEASE
 LDFLAGS += -Wl,-S,-x,-dead_strip_dylibs,-no_pie
else
 LDFLAGS += -Wl,-x,-dead_strip_dylibs,-no_pie
endif

CPPFLAGS += -I$(IMAGINE_PATH)/bundle/darwin-iOS/include
noDoubleFloat=1

# clang SVN doesn't seem to handle ASM properly so use as directly
AS := as
ASMFLAGS :=
