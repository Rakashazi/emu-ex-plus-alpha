include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ARCH := armv6
android_abi := armeabi
# -O2 seems to give slightly better performance with GCC 4.4.0
#HIGH_OPTIMIZE_CFLAGS = -O2 $(NORMAL_OPTIMIZE_CFLAGS_MISC) -funsafe-loop-optimizations -Wunsafe-loop-optimizations

android_cpuFlags := -march=armv5te -mtune=xscale -msoft-float

ifdef arm_fpu
 android_cpuFlags += -mfpu=$(arm_fpu)
else
 # not compiling with fpu support so THUMB is ok, but ARM usually gives better performance
 #android_cpuFlags += -mthumb
 noFpu = 1
endif

#CPPFLAGS += -isystem $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include
android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/libs/armeabi/libstlport_static.a -lstdc++

system_externalSysroot := $(IMAGINE_PATH)/bundle/android/armv6
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib

include $(currPath)/android-arm.mk
