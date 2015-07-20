# Included by arch-specific Android makefiles

ENV := android
CROSS_COMPILE := 1
binStatic := 1
android_libm ?= -lm

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
ANDROID_CLANG_VERSION ?= 3.6
ANDROID_CLANG_TOOLCHAIN_BIN_PATH ?= $(wildcard $(ANDROID_NDK_PATH)/toolchains/llvm-$(ANDROID_CLANG_VERSION)/prebuilt/*/bin)

ifeq ($(origin CC), default)
 ifeq ($(config_compiler),clang)
  CC := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/clang
  CXX := $(CC)++
  AR := $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/llvm-ar
 else 
  CC := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-gcc
  CXX := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-g++
  AR := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-gcc-ar
 endif
 RANLIB := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-ranlib
 STRIP := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-strip
 OBJDUMP := $(ANDROID_GCC_TOOLCHAIN_BIN_PATH)/$(CHOST)-objdump
 toolchainEnvParams += RANLIB="$(RANLIB)" STRIP="$(STRIP)" OBJDUMP="$(OBJDUMP)"
else
 # TODO: user-defined compiler
 ifneq ($(findstring $(shell $(CC) -v), "clang version"),)
  $(info detected clang compiler)
  config_compiler = clang
 endif
endif

CFLAGS_OPTIMIZE_DEBUG_DEFAULT ?= -O2
compiler_noSanitizeAddress := 1
ifeq ($(config_compiler),clang)
 include $(buildSysPath)/clang.mk
 android_stdcxx := libcxx
 CFLAGS_TARGET += -target $(clangTarget) -gcc-toolchain $(ANDROID_GCC_TOOLCHAIN_PATH)
 CFLAGS_CODEGEN += -fno-integrated-as
else
 include $(buildSysPath)/gcc.mk
 android_stdcxx := gnu
endif

ifeq ($(android_stdcxx), gnu)
 android_stdcxxLibPath := $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs
 android_stdcxxLibName := libgnustl_static.a
else # libc++
 android_stdcxxLibPath := $(ANDROID_NDK_PATH)/sources/cxx-stl/llvm-libc++/libs
 android_stdcxxLibName := libc++_static.a
endif

android_stdcxxLib := $(android_stdcxxLibPath)/$(android_abi)$(android_hardFPExt)/$(android_stdcxxLibName)
ifeq ($(ARCH), arm)
 ifeq ($(android_armState),-mthumb)
  android_stdcxxLib := $(android_stdcxxLibPath)/$(android_abi)$(android_hardFPExt)/thumb/$(android_stdcxxLibName)
 endif
endif

pkg_stdcxxStaticLib := $(android_stdcxxLib)

ifdef ANDROID_APK_SIGNATURE_HASH
 CPPFLAGS += -DANDROID_APK_SIGNATURE_HASH=$(ANDROID_APK_SIGNATURE_HASH)
endif

CFLAGS_TARGET += $(android_cpuFlags) --sysroot=$(android_ndkSysroot) -no-canonical-prefixes
CFLAGS_CODEGEN += -ffunction-sections -fdata-sections \
-Wa,--noexecstack
ASMFLAGS += -Wa,--noexecstack $(android_cpuFlags)
LDFLAGS += -no-canonical-prefixes \
-Wl,--no-undefined,-z,noexecstack,-z,relro,-z,now
LDFLAGS_SO := -Wl,-soname,lib$(android_soName).so -shared
LDLIBS += -lgcc -lc $(android_libm)
CPPFLAGS += -DANDROID
LDFLAGS += -s -Wl,-O1,--gc-sections,--compress-debug-sections=zlib,--icf=all,--as-needed
