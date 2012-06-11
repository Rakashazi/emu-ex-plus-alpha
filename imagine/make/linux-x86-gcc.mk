include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
CC := gcc-4.7.0 # testing on new GCC
include $(currPath)/linux-gcc.mk

CPPFLAGS += -m32
LDFLAGS += -m32

system_externalSysroot := $(IMAGINE_PATH)/bundle/linux-x86/usr
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib
