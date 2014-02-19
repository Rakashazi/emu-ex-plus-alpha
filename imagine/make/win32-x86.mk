include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/win32.mk

ARCH := x86
COMPILE_FLAGS += -m32 -march=pentium4 -mtune=generic
LDFLAGS += -m32
ASMFLAGS += -m32

ifeq ($(origin CC), default)
 CC := i686-mingw32-gcc
endif

RANLIB ?= i686-mingw32-ranlib

extraSysroot := $(IMAGINE_PATH)/bundle/win32-x86
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
CPPFLAGS += -I$(extraSysroot)/include