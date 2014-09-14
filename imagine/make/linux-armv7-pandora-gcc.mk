include $(IMAGINE_PATH)/make/config.mk

SUBENV := pandora
CROSS_COMPILE := 1
ARCH := arm
SUBARCH := armv7
ifeq ($(origin CC), default)
 CC := arm-none-linux-gnueabi-gcc
 CXX := arm-none-linux-gnueabi-g++
 CHOST := arm-none-linux-gnueabi
endif
configDefs += CONFIG_MACHINE_PANDORA
IMAGINE_SDK_PLATFORM = $(ENV)-$(SUBARCH)-$(SUBENV)

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

WARNINGS_CFLAGS += -Wdouble-promotion
# fix warning from old DBUS & libpng headers
BASE_CXXFLAGS += -Wno-literal-suffix

COMPILE_FLAGS += -march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
LDFLAGS += -march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp

pandoraSDKSysroot := $(PNDSDK)/usr
PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(pandoraSDKSysroot)/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(pandoraSDKSysroot)/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(pandoraSDKSysroot)/lib

# don't use FORTIFY_SOURCE to avoid linking in newer glibc symbols
CPPFLAGS += -I$(pandoraSDKSysroot)/include \
 -include $(buildSysPath)/include/glibc29Symver.h \
 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

# link librt to avoid pulling in GLIBC 2.17+ clock functions
LDLIBS += -L$(pandoraSDKSysroot)/lib -Wl,-rpath-link=$(pandoraSDKSysroot)/lib -lrt

linuxEventLoop := epoll
x11GLWinSystem := egl
openGLESVersion ?= 2

ifdef O_LTO
 # -flto-partition=none seems to help .symver issues
 LDFLAGS += -flto-partition=none
endif
