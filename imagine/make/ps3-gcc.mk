include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
ENV := ps3
CROSS_COMPILE := 1
CHOST := powerpc64
targetExtension := .elf

ifndef target
 target = $(metadata_exec)
endif

SONY_SDK := /usr/local/cell
SONY_CC := wine $(SONY_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
PKG_NPDRM := wine $(SONY_SDK)/host-win32/bin/psn_package_npdrm.exe

ifeq ($(origin CC), default)
 CC := powerpc64-ps3-elf-gcc
endif

compiler_noSanitizeAddress := 1
include $(buildSysPath)/gcc.mk

LD := wine $(SONY_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe

#HIGH_OPTIMIZE_CFLAGS += -ffunction-sections -fdata-sections
WARNINGS_CXXFLAGS += -Wno-attributes
LDFLAGS += -s -Wl,--strip-unused-data
#-Wl,--sn-no-dtors

ps3CellPPUPath := $(SONY_SDK)/target/ppu
ps3CellPPULibPath := $(ps3CellPPUPath)/lib
CPPFLAGS += -mcpu=cell -mhard-float -fpermissive -nostdinc --sysroot=$(ps3CellPPUPath) \
-isystem $(SONY_SDK)/host-win32/ppu/lib/gcc/ppu-lv2/4.1.1/include -isystem $(ps3CellPPUPath)/include -isystem $(SONY_SDK)/target/common/include
CPPFLAGS += -include $(IMAGINE_PATH)/src/base/ps3/macros.h

system_externalSysroot := $(IMAGINE_PATH)/bundle/ps3
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib
LDFLAGS += $(if $(cxxRTTI),,-fno-rtti) $(if $(cxxExceptions),,-fno-exceptions) -mcpu=cell -mhard-float
noDoubleFloat=1