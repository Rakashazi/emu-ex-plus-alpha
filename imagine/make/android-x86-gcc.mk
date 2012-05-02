include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ARCH := x86
android_abi := none
ifndef android_minSDK
 android_minSDK := 9
endif
android_ndkArch := x86

ifeq ($(origin CC), default)
 CC := i686-android-linux-gcc
endif

COMPILE_FLAGS += #-funwind-tables
LDFLAGS += -nostartfiles

android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/libs/x86/libstlport_static.a -lstdc++

system_externalSysroot := $(IMAGINE_PATH)/bundle/android/x86
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib

include $(currPath)/android-gcc.mk