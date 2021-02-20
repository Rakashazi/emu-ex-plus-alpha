# Included by arch-specific Android makefiles

ENV := android
ENV_KERNEL := linux
CROSS_COMPILE := 1

ifeq ($(ANDROID_NDK_PATH),)
 $(error setAndroidNDKPath.mk was not included in base makefile)
endif

ANDROID_CLANG_TOOLCHAIN_PATH ?= $(wildcard $(ANDROID_NDK_PATH)/toolchains/llvm/prebuilt/*)
ANDROID_CLANG_TOOLCHAIN_BIN_PATH := $(ANDROID_CLANG_TOOLCHAIN_PATH)/bin

ifdef V
 $(info NDK Clang path: $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH))
endif

ifneq ($(wildcard $(ANDROID_NDK_PATH)/sysroot),)
 $(error your NDK contains a deprecated sysroot directory, please upgrade to at least r22)
endif

ifeq ($(android_ndkSDK), 9)
 android_ndkLinkSysroot := $(IMAGINE_PATH)/bundle/android-$(android_ndkSDK)/arch-$(android_ndkArch)
endif

ifdef android_ndkLinkSysroot
 VPATH += $(android_ndkLinkSysroot)/usr/lib$(android_libDirExt)
else
 VPATH += $(ANDROID_CLANG_TOOLCHAIN_PATH)/sysroot/usr/lib/$(CHOST)/$(android_ndkSDK)
endif

config_compiler ?= clang

ifeq ($(origin CC), default)
 CC := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/clang
 CXX := $(CC)++
 AR := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/llvm-ar
 RANLIB := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/llvm-ar s
 STRIP := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/llvm-strip
 OBJDUMP := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/llvm-objdump
 toolchainEnvParams += RANLIB="$(RANLIB)" STRIP="$(STRIP)" OBJDUMP="$(OBJDUMP)"
else
 # TODO: user-defined compiler
 ifneq ($(findstring $(shell $(CC) -v), "clang version"),)
  $(info detected clang compiler)
  config_compiler = clang
 endif
 $(error user-defined compiler not yet supported)
endif

CFLAGS_OPTIMIZE_DEBUG_DEFAULT ?= -O2

ifneq ($(config_compiler),clang)
 $(error config_compiler must be set to clang)
endif

include $(buildSysPath)/clang.mk
CFLAGS_TARGET += -target $(clangTarget)

# libc++
STDCXXLIB = -static-libstdc++

ifdef ANDROID_APK_SIGNATURE_HASH
 CPPFLAGS += -DANDROID_APK_SIGNATURE_HASH=$(ANDROID_APK_SIGNATURE_HASH)
endif

CFLAGS_TARGET += $(android_cpuFlags) -no-canonical-prefixes
CFLAGS_CODEGEN += -ffunction-sections -fdata-sections
ASMFLAGS ?= $(CFLAGS_TARGET) -Wa,--noexecstack
ifdef android_ndkLinkSysroot
 LDFLAGS_SYSTEM += --sysroot=$(android_ndkLinkSysroot)
endif
LDFLAGS_SYSTEM += -no-canonical-prefixes \
-Wl,--no-undefined,-z,noexecstack,-z,relro,-z,now
linkAction = -Wl,-soname,lib$(android_metadata_soName).so -shared
LDLIBS_SYSTEM += -lm
LDLIBS += $(LDLIBS_SYSTEM)
CPPFLAGS += -DANDROID
LDFLAGS_SYSTEM += -s \
-Wl,-O3,--gc-sections,--compress-debug-sections=$(COMPRESS_DEBUG_SECTIONS),--icf=all,--as-needed,--warn-shared-textrel,--fatal-warnings \
-Wl,--exclude-libs,libgcc.a,--exclude-libs,libgcc_real.a -Wl,--exclude-libs,libatomic.a

ifeq ($(android_ndkSDK), 9)
 # SDK 9 no longer supported since NDK r16, enable compatibilty work-arounds
 CPPFLAGS += -DANDROID_COMPAT_API
endif

ifndef RELEASE
 CFLAGS_CODEGEN += -funwind-tables
endif
