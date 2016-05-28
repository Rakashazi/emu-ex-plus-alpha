include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
SUBARCH := armv7
android_abi := armeabi-v7a
clangTarget := armv7-none-linux-androideabi
armv7CPUFlags ?= -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16
android_cpuFlags ?= $(armv7CPUFlags)
LDFLAGS_SYSTEM += -Wl,--fix-cortex-a8
android_armv7State ?= -mthumb
android_armState := $(android_armv7State)
android_cpuFlags += $(android_armv7State)

include $(buildSysPath)/android-arm.mk
