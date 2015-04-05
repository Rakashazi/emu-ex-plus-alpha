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

ifeq ($(origin CC), default)
 CC := i686-linux-android-gcc
 CXX := $(CC)
 CHOST := i686-linux-android
endif

include $(buildSysPath)/android-gcc.mk

ifeq ($(config_compiler),clang)
 android_cpuFlags ?= -target i686-none-linux-android
endif
