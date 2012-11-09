ENV := ios
CROSS_COMPILE := 1

ifndef targetDir
 ifdef O_RELEASE
  targetDir := target/iOS/bin-release
 else
  targetDir := target/iOS/bin-debug
 endif
endif

ifeq ($(config_compiler),gcc)
 ifeq ($(origin CC), default)
  CC := llvm-gcc
 endif
 include $(currPath)/gcc.mk
 WARNINGS_CFLAGS += -Wno-attributes # for attributes not understood by llvm-gcc
else
 # default to clang
 config_compiler := clang
 ifeq ($(origin CC), default)
  CC := clang
 endif
 include $(currPath)/clang.mk
endif

ifdef ios_linkerPath
 # define an alternate PATH when linking
 LD = PATH=$(ios_linkerPath):$(PATH) $(CC)
endif

ifdef RELEASE
 COMPILE_FLAGS += -DNS_BLOCK_ASSERTIONS
endif

 # base engine code needs at least iOS 3.1
minIOSVer := 3.1
ifeq ($(ARCH),x86)
 IOS_SYSROOT = /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.0.sdk
 IOS_FLAGS = -isysroot $(IOS_SYSROOT) -mios-simulator-version-min=$(minIOSVer) -fobjc-abi-version=2 -fobjc-legacy-dispatch
else
 #IOS_SYSROOT = /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.0.sdk
 IOS_SYSROOT = ~/Desktop/Xcode46-DP1.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk
 IOS_FLAGS = -isysroot $(IOS_SYSROOT) -miphoneos-version-min=$(minIOSVer)
endif
CPPFLAGS += $(IOS_FLAGS)
LDFLAGS += $(IOS_FLAGS)

ifneq ($(config_compiler),clang)
 # TODO: find clang equivalent of -fsingle-precision-constant?
 COMPILE_FLAGS += -fsingle-precision-constant -ftree-vectorize
endif
#COMPILE_FLAGS += -ftemplate-depth-100
ifndef ios_noDeadStrip
 LDFLAGS += -dead_strip
endif
ifdef RELEASE
 LDFLAGS += -Wl,-S,-x,-dead_strip_dylibs
endif
WHOLE_PROGRAM_CFLAGS := -fipa-pta -fwhole-program

CPPFLAGS += -I$(IMAGINE_PATH)/bundle/darwin-iOS/include
noDoubleFloat=1

# clang SVN doesn't seem to handle ASM properly so use as directly
AS := as
ASMFLAGS :=

# always link with STL for now
#include $(IMAGINE_PATH)/make/package/stdc++.mk