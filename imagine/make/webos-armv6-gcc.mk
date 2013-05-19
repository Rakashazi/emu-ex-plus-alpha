include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

SUBARCH := armv6
webos_cpuFlags := -mcpu=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp

include $(buildSysPath)/webos-gcc.mk

ifndef target
 target := armv6
endif

extraSysroot := $(IMAGINE_PATH)/bundle/webos/armv6
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(extraSysroot)
CPPFLAGS += -I$(extraSysroot)/include
LDLIBS += -L$(extraSysroot)/lib
