include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
ARCH := mips
CHOST := mipsel-linux-android
android_abi := mips
android_ndkSDK ?= 15
android_ndkArch := mips
clangTarget := mipsel-none-linux-android
android_cpuFlags := -EL -mips32 -mhard-float

include $(buildSysPath)/android-gcc.mk