include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := x86
CHOST := i686-linux-android
android_abi := x86
android_ndkSDK ?= 16
android_ndkArch := x86
android_cxxSupportLibs := -landroid_support
# Must declare min API 21 to compile with NDK r26+ headers
clangTarget := i686-none-linux-android21
CFLAGS_CODEGEN += -fPIC -mstackrealign

include $(buildSysPath)/android-gcc.mk
