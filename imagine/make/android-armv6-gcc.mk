include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
SUBARCH := armv6
android_abi := armeabi
ifndef MACHINE
 MACHINE := GENERIC_ARM
endif

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

ifeq ($(android_hasSDK9), 1)
 # No Android 2.3+ armv5te devices exist to my knowledge
 android_cpuFlags += -march=armv6 -msoft-float
else
 android_cpuFlags += -march=armv5te -mtune=xscale -msoft-float
endif

ifeq ($(config_compiler),clang)
 android_cpuFlags += -target armv5te-none-linux-androideabi
endif