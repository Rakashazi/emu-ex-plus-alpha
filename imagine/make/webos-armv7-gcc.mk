include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

SUBARCH := armv7
webos_cpuFlags := -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
#-mthumb #-march=armv7-a

include $(buildSysPath)/webos-gcc.mk

ifndef target
 ifeq ($(webos_osVersion), 3)
  target := 3-armv7
 else
  target := armv7
 endif
endif

extraSysroot := $(IMAGINE_PATH)/bundle/webos/armv7
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(extraSysroot)
CPPFLAGS += -I$(extraSysroot)/include
LDLIBS += -L$(extraSysroot)/lib
