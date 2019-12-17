include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := arm
SUBARCH = armv7
CHOST := arm-linux-androideabi
android_abi := armeabi-v7a
android_ndkSDK ?= 9
android_ndkArch := arm
# Must declare min API 14 to compile with unified headers
clangTarget := armv7-none-linux-androideabi14
CFLAGS_CODEGEN += -fpic
armv7CPUFlags ?= -march=armv7-a -mtune=generic -mfpu=vfpv3-d16
android_cpuFlags ?= $(armv7CPUFlags)
android_armv7State ?= -mthumb
android_armState := $(android_armv7State)
android_cpuFlags += $(android_armv7State)

include $(buildSysPath)/android-gcc.mk
