include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

MACHINE := OUYA

ifndef arm_fpu
 arm_fpu := neon
endif

ifndef targetDir
 ifdef O_RELEASE
  targetDir := target/android-ouya-$(android_minSDK)/libs-release/armeabi-v7a
 else
  targetDir := target/android-ouya-$(android_minSDK)/libs-debug/armeabi-v7a
 endif
endif

include $(buildSysPath)/android-armv7-gcc.mk

android_cpuFlags += -mcpu=cortex-a9
