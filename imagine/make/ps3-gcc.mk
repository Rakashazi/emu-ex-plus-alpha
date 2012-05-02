include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ENV := ps3
CHOST := powerpc64
targetExtension := .elf

ifeq ($(origin CC), default)
 CC := wine /usr/local/cell/host-win32/ppu/bin/ppu-lv2-gcc.exe
 #CC := powerpc64-ps3-elf-gcc
endif

include $(currPath)/gcc.mk

#LD := wine /usr/local/cell/host-win32/ppu/bin/ppu-lv2-gcc.exe

HIGH_OPTIMIZE_CFLAGS += -ffunction-sections -fdata-sections
WARNINGS_CXXFLAGS += -Wno-non-virtual-dtor -Wno-attributes
LDFLAGS += -s -Wl,--strip-unused-data

system_externalSysroot := $(IMAGINE_PATH)/bundle/ps3/usr
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib
noDoubleFloat=1