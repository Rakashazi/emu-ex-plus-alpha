include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

SUBARCH := armv6
webos_cpuFlags := -mcpu=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp

include $(currPath)/webos-gcc.mk

ifndef target
 target := armv6
endif

system_externalSysroot := $(IMAGINE_PATH)/bundle/webos/armv6
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib
