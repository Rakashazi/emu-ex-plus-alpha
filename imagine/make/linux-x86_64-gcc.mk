include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(buildSysPath)/linux-gcc.mk

ARCH := x86_64
ifeq ($(config_compiler),clang)
 COMPILE_FLAGS += -march=x86-64
else
 COMPILE_FLAGS += -m64 -mtune=generic
 LDFLAGS += -m64
 ASMFLAGS += -m64
endif

ifdef extraSysroot
 PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
 #PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include
 #PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib
 #CPPFLAGS += -I$(extraSysroot)/include
 #LDLIBS += -L$(extraSysroot)/lib
endif
