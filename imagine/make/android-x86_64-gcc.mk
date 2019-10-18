include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := x86_64
CHOST := x86_64-linux-android
android_abi := x86_64
android_ndkSDK ?= 21
android_ndkArch := x86_64
clangTarget := x86_64-none-linux-android21
CFLAGS_CODEGEN += -fPIC
ANDROID_GCC_TOOLCHAIN_ROOT_DIR := x86_64
android_libDirExt := 64

include $(buildSysPath)/android-gcc.mk
