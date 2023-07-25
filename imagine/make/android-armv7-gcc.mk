include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := arm
SUBARCH = armv7
CHOST := arm-linux-androideabi
android_abi := armeabi-v7a
android_ndkSDK ?= 9
android_ndkArch := arm
# Must declare min API 21 to compile with NDK r26+ headers
clangTarget := armv7-none-linux-androideabi21
CFLAGS_CODEGEN += -fpic
armv7CPUFlags ?= -march=armv7-a -mtune=generic
android_cpuFlags ?= $(armv7CPUFlags)
android_armv7State ?= -mthumb
android_armState := $(android_armv7State)
android_cpuFlags += $(android_armv7State)
android_cxxSupportLibs := -landroid_support
ASMFLAGS = --noexecstack -EL -mfloat-abi=softfp -march=armv7-a
LDFLAGS_SYSTEM += -Wl,--fix-cortex-a8

include $(buildSysPath)/android-gcc.mk

# Directly call the GNU assembler until assembly in projects is updated for clang's integrated assembler

ifneq ($(shell which arm-none-linux-gnueabi-as),)
 AS = arm-none-linux-gnueabi-as
else
 AS = arm-linux-gnueabi-as
endif
