include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

ARCH := x86
include $(currPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -x86
endif

minIOSVer := 5.0
IOS_FLAGS += -arch i386
ASMFLAGS += -arch i386
CHOST := $(shell $(CC) -arch i386 -dumpmachine)
system_externalSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/x86
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib