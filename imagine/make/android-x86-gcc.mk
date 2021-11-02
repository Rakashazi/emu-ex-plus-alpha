include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := x86
CHOST := i686-linux-android
android_abi := x86
android_ndkSDK ?= 16
android_ndkArch := x86
# Must declare min API 19 to compile with NDK r24+ headers
clangTarget := i686-none-linux-android19
CFLAGS_CODEGEN += -fPIC -mstackrealign

include $(buildSysPath)/android-gcc.mk
