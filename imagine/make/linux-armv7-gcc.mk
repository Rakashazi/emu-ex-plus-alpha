include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/linux-gcc.mk

COMPILE_FLAGS += -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
LDFLAGS += -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp

system_externalSysroot := $(IMAGINE_PATH)/bundle/linux-armv7-ubuntu-natty
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib