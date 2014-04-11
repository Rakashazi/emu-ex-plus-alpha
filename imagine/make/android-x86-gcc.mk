include $(IMAGINE_PATH)/make/config.mk
ARCH := x86
android_abi := x86
ifndef android_minSDK
 android_minSDK := 9
endif
# TODO: android_minSDK should only apply to APK metadata
ifndef android_minLibSDK
 android_minLibSDK := 15
endif
android_ndkArch := x86
ifndef MACHINE
 MACHINE := GENERIC_X86
endif

ifeq ($(origin CC), default)
 CC := i686-linux-android-gcc
 CXX := $(CC)
 CHOST := i686-linux-android
endif

# CPU flags normally patched in as defaults on official Android GCC
android_cpuFlags += -march=i686 -mtune=atom -mstackrealign -msse3 -mfpmath=sse -m32 #-fPIC
COMPILE_FLAGS += -fno-short-enums

# TODO: shared object creation not working correctly in custom GCC 4.7 toolchain,
# maybe a patch is missing? Send the parameters directly to linker for now
#LDLIBS += -Wl,-shared,-dynamic-linker,/system/bin/linker,-X

include $(buildSysPath)/android-gcc.mk

ifeq ($(config_compiler),clang)
 android_cpuFlags += -target i686-none-linux-android
endif

openGLESVersion ?= 2
