include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/linux-gcc.mk

ifeq ($(config_compiler),clang)
 COMPILE_FLAGS += -march=x86-64
else
 COMPILE_FLAGS += -m64
 LDFLAGS += -m64
endif

system_externalSysroot := $(IMAGINE_PATH)/bundle/linux-x86_64/usr
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib
