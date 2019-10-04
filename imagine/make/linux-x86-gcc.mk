include $(IMAGINE_PATH)/make/config.mk

include $(buildSysPath)/linux-gcc.mk

CHOST := $(shell $(CC) -dumpmachine)
ARCH := x86
CFLAGS_CODEGEN += -m32 -march=pentium4 -mtune=generic
LDFLAGS_SYSTEM += -m32
ASMFLAGS += -m32

ifneq ($(filter x86_64-%,$(CHOST)),)
 x86PkgConfigPath ?= /usr/lib/pkgconfig
 PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(x86PkgConfigPath)
endif
