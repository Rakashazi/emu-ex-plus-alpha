include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/win32.mk

ARCH := x86
CFLAGS_CODEGEN += -m32 -march=pentium4 -mtune=generic
LDFLAGS += -m32
ASMFLAGS += -m32

ifeq ($(origin CC), default)
 CC := i686-mingw32-gcc
 CXX := i686-mingw32-g++
endif

RANLIB ?= i686-mingw32-ranlib
