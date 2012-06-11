include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ARCH := armv6
android_abi := armeabi

android_cpuFlags := -march=armv5te -mtune=xscale -msoft-float

ifdef arm_fpu
 android_cpuFlags += -mfpu=$(arm_fpu)
else
 # not compiling with fpu support so THUMB is ok, but ARM usually gives better performance
 #android_cpuFlags += -mthumb
 noFpu = 1
endif

system_externalSysroot := $(IMAGINE_PATH)/bundle/android/armv6
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib

include $(currPath)/android-arm.mk
