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
armv7CPUFlags ?= -march=armv7-a -mtune=generic
android_cpuFlags ?= $(armv7CPUFlags)
android_armv7State ?= -mthumb
android_armState := $(android_armv7State)
android_cpuFlags += $(android_armv7State)
ASMFLAGS = --noexecstack -EL -mfloat-abi=softfp -march=armv7-a

include $(buildSysPath)/android-gcc.mk

# Directly call the GNU assembler until assembly in projects is updated for clang's integrated assembler
AS = $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/arm-linux-androideabi-as
