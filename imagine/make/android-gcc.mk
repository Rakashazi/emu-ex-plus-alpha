# Included by arch-specific Android makefiles

ENV := android
CROSS_COMPILE := 1
binStatic := 1

ifeq ($(wildcard $(ANDROID_NDK_PATH)/platforms)),)
 $(error Invalid NDK path:$(ANDROID_NDK_PATH), add NDK directory to PATH, define ANDROID_NDK_PATH, or move NDK to the default path:$(defaultNDKPath)
endif

android_ndkSysroot := $(ANDROID_NDK_PATH)/platforms/android-$(android_ndkSDK)/arch-$(android_ndkArch)

VPATH += $(android_ndkSysroot)/usr/lib

android_soName := main

ANDROID_GCC_VERSION ?= 4.9
ANDROID_GCC_TOOLCHAIN_ROOT_DIR ?= $(CHOST)
ANDROID_GCC_TOOLCHAIN_PATH ?= $(wildcard $(ANDROID_NDK_PATH)/toolchains/$(ANDROID_GCC_TOOLCHAIN_ROOT_DIR)-$(ANDROID_GCC_VERSION)/prebuilt/*)
ifeq ($(ANDROID_GCC_TOOLCHAIN_PATH),)
 $(error missing Android GCC toolchain at path:$(ANDROID_GCC_TOOLCHAIN_PATH))
endif
ANDROID_GCC_TOOLCHAIN_BIN_PATH := $(ANDROID_GCC_TOOLCHAIN_PATH)/bin
ANDROID_CLANG_TOOLCHAIN_BIN_PATH ?= $(wildcard $(ANDROID_NDK_PATH)/toolchains/llvm/prebuilt/*/bin)

ifdef V
 $(info NDK GCC path: $(ANDROID_GCC_TOOLCHAIN_BIN_PATH))
 $(info NDK Clang path: $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH))
endif

config_compiler ?= clang

ifeq ($(origin CC), default)
 ifeq ($(config_compiler),clang)
  CC := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/clang
  CXX := $(CC)++
  AR := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-ar
  RANLIB := ranlib
 else
  CC := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-gcc
  CXX := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-g++
  AR := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-gcc-ar
  RANLIB := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-ranlib
 endif
 STRIP := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-strip
 OBJDUMP := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-objdump
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
compiler_noSanitizeAddress := 1
ifeq ($(config_compiler),clang)
 include $(buildSysPath)/clang.mk
 android_stdcxx ?= libcxx
 CFLAGS_TARGET += -target $(clangTarget) -gcc-toolchain $(ANDROID_GCC_TOOLCHAIN_PATH)
 CFLAGS_CODEGEN += -fno-integrated-as
 ASMFLAGS += -fno-integrated-as
 # -flto needed to enable linking any static archives with LTO 
 LDFLAGS_SYSTEM += -flto $(CFLAGS_CODEGEN)
 
 # check for llvm-ar
 ifeq ($(shell which $(AR)),)
  $(error missing llvm-ar, make sure LLVM is installed (Android NDK's LLVM doesn't provide this))
 endif
else
 include $(buildSysPath)/gcc.mk
 android_stdcxx ?= gnu
 CFLAGS_CODEGEN += -Wa,--noexecstack
endif

ifeq ($(android_stdcxx), gnu)
 android_stdcxxLibPath := $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(ANDROID_GCC_VERSION)/libs
 android_stdcxxLibName := libgnustl_static.a
 android_stdcxxLib := $(android_stdcxxLibPath)/$(android_abi)/$(android_stdcxxLibName)
else # libc++
 android_stdcxxLibPath := $(ANDROID_NDK_PATH)/sources/cxx-stl/llvm-libc++/libs
 android_stdcxxLibName := libc++_static.a
 android_stdcxxLibArchPath := $(android_stdcxxLibPath)/$(android_abi)
 android_stdcxxLib := $(android_stdcxxLibArchPath)/$(android_stdcxxLibName) \
 $(android_stdcxxLibArchPath)/libc++abi.a \
 $(android_stdcxxLibArchPath)/libandroid_support.a
 ifeq ($(ARCH), arm)
  android_stdcxxLib += $(android_stdcxxLibArchPath)/libunwind.a
 endif
endif

pkg_stdcxxStaticLib := $(android_stdcxxLib)

ifdef ANDROID_APK_SIGNATURE_HASH
 CPPFLAGS += -DANDROID_APK_SIGNATURE_HASH=$(ANDROID_APK_SIGNATURE_HASH)
endif

CFLAGS_TARGET += $(android_cpuFlags) --sysroot=$(android_ndkSysroot) -no-canonical-prefixes
CFLAGS_CODEGEN += -ffunction-sections -fdata-sections
ASMFLAGS += $(CFLAGS_TARGET) -Wa,--noexecstack
LDFLAGS_SYSTEM += -no-canonical-prefixes \
-Wl,--no-undefined,-z,noexecstack,-z,relro,-z,now
linkAction := -Wl,-soname,lib$(android_soName).so -shared
ifneq ($(config_compiler),clang)
 LDLIBS_SYSTEM += -lc -lgcc
endif
LDLIBS_SYSTEM += -lm
LDLIBS += $(LDLIBS_SYSTEM)
CPPFLAGS += -DANDROID
LDFLAGS_SYSTEM += -s -Wl,-O1,--gc-sections,--compress-debug-sections=zlib,--icf=all,--as-needed

ifndef RELEASE
 CFLAGS_CODEGEN += -funwind-tables
endif
