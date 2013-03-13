include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

SUBENV := pandora
CROSS_COMPILE := 1
ARCH := arm
SUBARCH := armv7
ifeq ($(origin CC), default)
 CC := arm-none-linux-gnueabi-gcc
endif
config_gfx_openGLES := 1
noDoubleFloat := 1
configDefs += CONFIG_MACHINE_OPEN_PANDORA

include $(currPath)/linux-gcc.mk

COMPILE_FLAGS += -fsingle-precision-constant
WARNINGS_CFLAGS += -Wdouble-promotion

COMPILE_FLAGS += -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
LDFLAGS += -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp

system_externalSysroot := $(HOME)/pandora-dev/arm-2011.09/usr
CPPFLAGS += -idirafter $(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib  -Wl,-rpath-link=$(system_externalSysroot)/lib $(system_externalSysroot)/lib/libm.so