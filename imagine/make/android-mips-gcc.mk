include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ARCH := mips
android_abi := mips
ifndef MACHINE
 MACHINE := GENERIC_MIPS
endif

android_cpuFlags := -EL -mips32 -mhard-float

android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/libs/mips/libstlport_static.a -lstdc++

extraSysroot := $(IMAGINE_PATH)/bundle/android/mips
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(extraSysroot)
CPPFLAGS += -I$(extraSysroot)/include
LDLIBS += -L$(extraSysroot)/lib

ifndef android_minSDK 
 android_minSDK := 9
endif
android_ndkArch := mips

ifeq ($(origin CC), default)
 CC := mipsel-linux-android-gcc
endif

CPPFLAGS += -D__ANDROID__
COMPILE_FLAGS += -fno-short-enums
LDLIBS += -nostartfiles
noDoubleFloat=1

include $(buildSysPath)/android-gcc.mk