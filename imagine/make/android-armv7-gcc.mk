include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ARCH := armv7
android_abi := armeabi-v7a

ifndef arm_fpu
 arm_fpu := vfp
endif

ifndef android_armv7State
 android_armv7State := -mthumb
endif

android_cpuFlags := $(android_armv7State) -march=armv7-a -mfloat-abi=softfp -mfpu=$(arm_fpu)

system_externalSysroot := $(IMAGINE_PATH)/bundle/android/armv7
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib

include $(currPath)/android-arm.mk
