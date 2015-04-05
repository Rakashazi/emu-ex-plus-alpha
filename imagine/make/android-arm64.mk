include $(IMAGINE_PATH)/make/config.mk
ARCH := aarch64
SUBARCH = arm64
android_abi := arm64-v8a
android_minSDK ?= 9
android_minLibSDK ?= 21
android_ndkArch := arm64

ifeq ($(origin CC), default)
 CC := aarch64-linux-android-gcc
 CXX := $(CC)
 CHOST := aarch64-linux-android
endif

COMPILE_FLAGS += -fpic

include $(buildSysPath)/android-gcc.mk

ifeq ($(config_compiler),clang)
 android_cpuFlags ?= -target aarch64-none-linux-android
endif
