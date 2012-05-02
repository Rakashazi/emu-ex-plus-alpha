include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ARCH := armv7
android_abi := armeabi-v7a

ifndef arm_fpu
 arm_fpu := vfp
endif

android_cpuFlags := -mthumb -march=armv7-a -mfloat-abi=softfp -mfpu=$(arm_fpu)

#CPPFLAGS += -isystem $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/include
android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/libs/armeabi-v7a/libstlport_static.a -lstdc++

system_externalSysroot := $(IMAGINE_PATH)/bundle/android/armv7
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib

include $(currPath)/android-arm.mk
