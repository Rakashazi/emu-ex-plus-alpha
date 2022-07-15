include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := arm
SUBARCH = armv7
CHOST := arm-linux-androideabi
android_abi := armeabi-v7a
android_ndkSDK ?= 9
android_ndkArch := arm
# Must declare min API 19 to compile with NDK r24+ headers
clangTarget := armv7-none-linux-androideabi19
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

ifneq ($(wildcard $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/arm-linux-androideabi-as),) # check for GNU assember included in NDK <= r23
 AS = $(ANDROID_CLANG_TOOLCHAIN_BIN_PATH)/arm-linux-androideabi-as
else
 ifneq ($(shell which arm-none-linux-gnueabi-as),)
  AS = arm-none-linux-gnueabi-as
 else
  AS = arm-linux-gnueabi-as
 endif
endif
