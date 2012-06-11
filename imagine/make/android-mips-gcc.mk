include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ARCH := mips
android_abi := mips

android_cpuFlags := -EL -mips32 -mhard-float

android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/libs/mips/libstlport_static.a -lstdc++

system_externalSysroot := $(IMAGINE_PATH)/bundle/android/mips
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib

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

include $(currPath)/android-gcc.mk