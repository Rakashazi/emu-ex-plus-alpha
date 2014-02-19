include $(IMAGINE_PATH)/make/config.mk
ARCH := mips
android_abi := mips
ifndef MACHINE
 MACHINE := GENERIC_MIPS
endif

android_cpuFlags := -EL -mips32 -mhard-float

extraSysroot := $(IMAGINE_PATH)/bundle/android/mips
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
CPPFLAGS += -I$(extraSysroot)/include

ifndef android_minSDK 
 android_minSDK := 9
endif
android_ndkArch := mips

ifeq ($(origin CC), default)
 CC := mipsel-linux-android-gcc
endif

CPPFLAGS += -D__ANDROID__
COMPILE_FLAGS += -fno-short-enums
LDLIBS += -nostartfiles
noDoubleFloat=1

include $(buildSysPath)/android-gcc.mk