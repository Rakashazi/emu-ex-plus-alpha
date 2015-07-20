include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/setAndroidNDKPath.mk
SUBARCH := armv6
android_abi := armeabi
clangTarget := armv5te-none-linux-androideabi

include $(buildSysPath)/android-arm.mk

# No Android 2.3+ armv5te devices exist to my knowledge
armCPUFlags := -march=armv6 -mfpu=vfp -mno-unaligned-access -msoft-float
android_cpuFlags ?= $(armCPUFlags) 
