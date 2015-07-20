include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := x86
CHOST := i686-linux-android
android_abi := x86
android_ndkSDK ?= 15
android_ndkArch := x86
clangTarget := i686-none-linux-android
ANDROID_GCC_TOOLCHAIN_ROOT_DIR := x86

include $(buildSysPath)/android-gcc.mk
