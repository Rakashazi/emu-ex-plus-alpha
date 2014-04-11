include $(IMAGINE_PATH)/make/config.mk

SUBARCH := armv6
webos_cpuFlags := -mcpu=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp

include $(buildSysPath)/webos-gcc.mk

ifndef target
 target := armv6
endif

openGLESVersion := 1