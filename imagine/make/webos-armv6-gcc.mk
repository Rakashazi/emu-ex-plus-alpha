include $(IMAGINE_PATH)/make/config.mk

SUBARCH := armv6
webos_cpuFlags := -mcpu=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp

include $(buildSysPath)/webos-gcc.mk

ifndef target
 target := armv6
endif

extraSysroot := $(IMAGINE_PATH)/bundle/webos/armv6
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
CPPFLAGS += -I$(extraSysroot)/include

openGLESVersion := 1