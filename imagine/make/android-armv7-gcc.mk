include $(IMAGINE_PATH)/make/config.mk
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
CPPFLAGS += -I$(extraSysroot)/include
LDFLAGS += -Wl,--fix-cortex-a8

android_hardFP ?= 1

ifeq ($(android_hardFP),1)
 android_cpuFlags += -mhard-float
 # NOTE: do not also link in -lm or strange runtime behavior can result, especially with LTO
 android_libm := -lm_hard
 CPPFLAGS += -D_NDK_MATH_NO_SOFTFP=1
 LDFLAGS += -Wl,--no-warn-mismatch
 android_hardFPExt := _hard
endif

include $(buildSysPath)/android-arm.mk

ifeq ($(config_compiler),clang)
 android_cpuFlags += -target armv7-none-linux-androideabi
endif

openGLESVersion ?= 2
