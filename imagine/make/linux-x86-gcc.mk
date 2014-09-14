include $(IMAGINE_PATH)/make/config.mk

# gcc-ar on GCC 4.8.2 & binutils 2.24.51.0.3 seems to segfault with slim-LTO, disable for now
# TODO: re-test in future version of GCC and/or binutils
O_LTO_FAT := 1

include $(buildSysPath)/linux-gcc.mk

CHOST := $(shell $(CC) -dumpmachine)
ARCH := x86
COMPILE_FLAGS += -m32 -march=pentium4 -mtune=generic
LDFLAGS += -m32
ASMFLAGS += -m32

ifneq ($(filter x86_64-%,$(CHOST)),)
 x86PkgConfigPath ?= /usr/lib32/pkgconfig
 PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(x86PkgConfigPath)
endif
