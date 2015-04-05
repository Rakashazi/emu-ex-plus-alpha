include $(IMAGINE_PATH)/make/config.mk
ARCH := mips
android_abi := mips

android_cpuFlags := -EL -mips32 -mhard-float

ifndef android_minSDK 
 android_minSDK := 9
endif
android_ndkArch := mips

ifeq ($(origin CC), default)
 CC := mipsel-linux-android-gcc
 CXX := $(CC)
endif

CPPFLAGS += -D__ANDROID__
COMPILE_FLAGS += -fno-short-enums
LDLIBS += -nostartfiles

include $(buildSysPath)/android-gcc.mk