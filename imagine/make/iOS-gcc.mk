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
CFLAGS_WARN += -Wno-error=deprecated-declarations

ifdef CCTOOLS_TOOCHAIN_PATH
AR := llvm-ar
RANLIB := $(AR) s
# Use --no-default-config to prevent distro's Clang config from adding its flags to the build
CC := $(firstword $(wildcard $(CCTOOLS_TOOCHAIN_PATH)/bin/*-clang)) --no-default-config
CXX := $(firstword $(wildcard $(CCTOOLS_TOOCHAIN_PATH)/bin/*-clang++)) --no-default-config
LD := $(CXX)
iosSimulatorSDKsPath := $(CCTOOLS_TOOCHAIN_PATH)/SDK
iosSDKsPath := $(CCTOOLS_TOOCHAIN_PATH)/SDK
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

ifndef ios_noDeadStrip
 LDFLAGS_SYSTEM += -dead_strip
endif
ifdef RELEASE
 LDFLAGS_SYSTEM += -Wl,-S,-x,-dead_strip_dylibs
else
 LDFLAGS_SYSTEM += -Wl,-x,-dead_strip_dylibs
endif
LDFLAGS += -Wl,-no_pie

# libc++
ios_useExternalLibcxx := 1
ifdef ios_useExternalLibcxx
 ifneq ($(pkgName),libcxx) # check we aren't building lib++ itself
  STDCXXLIB = -nostdlib++ -lc++ -lc++abi -lc++experimental
  CPPFLAGS += -nostdinc++ -I$(IMAGINE_SDK_PLATFORM_PATH)/include/c++/v1 -D_LIBCPP_DISABLE_AVAILABILITY
 else
  CPPFLAGS += -stdlib=libc++
 endif
else
 STDCXXLIB = -stdlib=libc++
 CXXFLAGS_LANG += -stdlib=libc++ -D_LIBCPP_DISABLE_AVAILABILITY
 ifdef CCTOOLS_TOOCHAIN_PATH
  CPPFLAGS += -I$(firstword $(wildcard $(iosSDKsPath)/iPhoneOS*.sdk/usr/include/c++))
 endif
endif

# clang SVN doesn't seem to handle ASM properly so use as directly
AS := as
ASMFLAGS :=
