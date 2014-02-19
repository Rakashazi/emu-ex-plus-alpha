include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/linux-gcc.mk

CHOST := $(shell $(CC) -dumpmachine)
ARCH := x86
COMPILE_FLAGS += -m32 -march=pentium4 -mtune=generic
LDFLAGS += -m32
ASMFLAGS += -m32

ifdef extraSysroot
 PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
 CPPFLAGS += -I$(extraSysroot)/include
endif

ifneq ($(filter x86_64-%,$(CHOST)),)
 x86PkgConfigPath ?= /usr/lib32/pkgconfig
 PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(x86PkgConfigPath)
endif

linuxEventLoop := glib