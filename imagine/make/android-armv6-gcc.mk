include $(IMAGINE_PATH)/make/config.mk
SUBARCH := armv6
android_abi := armeabi

include $(buildSysPath)/android-arm.mk

# No Android 2.3+ armv5te devices exist to my knowledge
armv6CPUFlags := -march=armv6  -mno-unaligned-access -msoft-float

ifeq ($(config_compiler),clang)
 android_cpuFlags ?= -target armv5te-none-linux-androideabi $(armv6CPUFlags)
else
 android_cpuFlags ?= $(armv6CPUFlags) 
endif
