include $(IMAGINE_PATH)/make/config.mk

ARCH := x86
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -x86
endif

minIOSVer = 5.0
IOS_FLAGS += -arch i386
ASMFLAGS += -arch i386
COMPILE_FLAGS += -mdynamic-no-pic
CHOST := $(shell $(CC) -arch i386 -dumpmachine)

extraSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/x86
PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
CPPFLAGS += -I$(extraSysroot)/include