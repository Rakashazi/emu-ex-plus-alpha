include $(IMAGINE_PATH)/make/config.mk

SUBENV := pandora
CROSS_COMPILE := 1
ARCH := arm
SUBARCH := armv7
ifeq ($(origin CC), default)
 CHOST := arm-none-linux-gnueabi
 CC := $(CHOST)-gcc-14
 CXX := $(CHOST)-g++-14
endif
configEnable += CONFIG_MACHINE_PANDORA
IMAGINE_SDK_PLATFORM = $(ENV)-$(SUBARCH)-$(SUBENV)

ifndef buildName
 ifdef O_RELEASE
  buildName := pandora-release
 else
  buildName := pandora
 endif
endif

targetDir ?= target/$(buildName)

openGLAPI := gles
staticLibcxx := 1
include $(buildSysPath)/linux-gcc.mk

CFLAGS_WARN += -Wno-psabi -Wno-unused-value
CXXFLAGS_WARN += -Wno-register

CFLAGS_CODEGEN += -mcpu=cortex-a8 -mfpu=neon -fno-stack-protector
LDFLAGS_SYSTEM += -mcpu=cortex-a8 -mfpu=neon -fno-stack-protector --sysroot=$(pandoraSDKSysroot) -s

pandoraSDKSysroot := $(PNDSDK)
PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(pandoraSDKSysroot)/usr/lib/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(pandoraSDKSysroot)/usr/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH := $(pandoraSDKSysroot)/usr/lib

# don't use FORTIFY_SOURCE to avoid linking in newer glibc symbols
CPPFLAGS += --sysroot=$(pandoraSDKSysroot) \
 -isystem /usr/lib/gcc/$(CHOST)/14/include/g++-v14 \
 -isystem /usr/$(CHOST)/usr/include \
 -isystem $(pandoraSDKSysroot)/usr/include \
 -I$(IMAGINE_SDK_PLATFORM_PATH)/include \
 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

LDLIBS += -Wl,-rpath-link=$(pandoraSDKSysroot)/usr/lib -lrt