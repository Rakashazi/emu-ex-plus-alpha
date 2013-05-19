include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
SUBARCH := armv7
android_abi := armeabi-v7a
ifndef MACHINE
 MACHINE := GENERIC_ARMV7
endif

ifndef arm_fpu
 arm_fpu := vfpv3-d16
endif

ifndef android_armv7State
 android_armv7State := -mthumb
endif
android_armState := $(android_armv7State)

android_cpuFlags := $(android_armv7State) -march=armv7-a -mfloat-abi=softfp -mfpu=$(arm_fpu)

extraSysroot := $(IMAGINE_PATH)/bundle/android/armv7
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(extraSysroot)
CPPFLAGS += -I$(extraSysroot)/include
LDLIBS += -L$(extraSysroot)/lib

include $(buildSysPath)/android-arm.mk

ifeq ($(config_compiler),clang)
 android_cpuFlags += -target armv7-none-linux-androideabi
endif