include $(IMAGINE_PATH)/make/config.mk
SUBARCH := armv7
android_abi := armeabi-v7a

armv7CPUFlags ?= -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16
LDFLAGS += -Wl,--fix-cortex-a8

android_hardFP ?= 1

include $(buildSysPath)/android-arm.mk

ifeq ($(config_compiler),clang)
 android_cpuFlags ?= -target armv7-none-linux-androideabi $(armv7CPUFlags)
else
 android_cpuFlags ?= $(armv7CPUFlags)
endif

ifeq ($(android_hardFP),1)
 android_cpuFlags += -mhard-float
 # NOTE: do not also link in -lm or strange runtime behavior can result, especially with LTO
 android_libm := -lm_hard
 CPPFLAGS += -D_NDK_MATH_NO_SOFTFP=1
 LDFLAGS += -Wl,--no-warn-mismatch
 android_hardFPExt := -hard
endif

android_armv7State ?= -mthumb
android_armState := $(android_armv7State)
android_cpuFlags += $(android_armv7State)