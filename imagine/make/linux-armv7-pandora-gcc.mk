include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

SUBENV := pandora
CROSS_COMPILE := 1
ARCH := arm
SUBARCH := armv7
ifeq ($(origin CC), default)
 CC := arm-none-linux-gnueabi-gcc
endif
config_gfx_openGLES := 1
noDoubleFloat := 1
configDefs += CONFIG_MACHINE_PANDORA

ifndef buildName
 ifdef O_RELEASE
  buildName := pandora-release
 else
  buildName := pandora
 endif
endif

targetDir ?= target/$(buildName)

compiler_noSanitizeAddress := 1
staticLibcxx := 1
include $(buildSysPath)/linux-gcc.mk

COMPILE_FLAGS += -fsingle-precision-constant
WARNINGS_CFLAGS += -Wdouble-promotion
# fix warning on old DBUS headers
BASE_CXXFLAGS += -Wno-literal-suffix

COMPILE_FLAGS += -march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
LDFLAGS += -march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp

pandoraSDKSysroot := $(PNDSDK)/usr
extraSysroot := $(IMAGINE_PATH)/bundle/linux-armv7-pandora
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig:$(pandoraSDKSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(extraSysroot)/include:$(pandoraSDKSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(extraSysroot)/lib:$(pandoraSDKSysroot)/lib
pkgConfigOpts := --define-variable=prefix=$(pandoraSDKSysroot)
CPPFLAGS += -I$(extraSysroot)/include -I$(pandoraSDKSysroot)/include \
 -include $(IMAGINE_PATH)/src/config/glibc29Symver.h \
 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
# don't use FORTIFY_SOURCE to avoid linking in newer glibc symbols 
LDLIBS += -L$(extraSysroot)/lib \
 -L$(pandoraSDKSysroot)/lib -Wl,-rpath-link=$(pandoraSDKSysroot)/lib

ifdef O_LTO
 # -flto-partition=none seems to help .symver issues
 LDFLAGS += -flto-partition=none
endif
