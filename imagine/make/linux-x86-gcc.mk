include $(IMAGINE_PATH)/make/config.mk

compiler_sanitizeMode ?= address,undefined
include $(buildSysPath)/linux-gcc.mk

CHOST := $(shell $(CC) -dumpmachine)
ARCH := x86
ifneq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := gcc-14
  CXX := g++-14
 endif
endif

CFLAGS_CODEGEN += -m32 -march=pentium4 -mtune=generic
LDFLAGS_SYSTEM += -m32
ASMFLAGS += -m32

ifneq ($(filter x86_64-%,$(CHOST)),)
 x86PkgConfigPath ?= /usr/lib/pkgconfig
 PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(x86PkgConfigPath)
endif
