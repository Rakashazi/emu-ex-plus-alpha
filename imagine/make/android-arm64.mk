include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := aarch64
SUBARCH = arm64
CHOST := aarch64-linux-android
android_abi := arm64-v8a
android_ndkSDK ?= 21
android_ndkArch := arm64
clangTarget := aarch64-none-linux-android21
CFLAGS_CODEGEN += -fpic

include $(buildSysPath)/android-gcc.mk
