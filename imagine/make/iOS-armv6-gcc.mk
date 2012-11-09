include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk

ARCH := arm
SUBARCH := armv6
include $(currPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -armv6
endif

IOS_SYSROOT = /Applications/Xcode44.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.1.sdk
IOS_FLAGS += -arch armv6
ASMFLAGS += -arch armv6
CHOST := $(shell $(CC) -arch armv6 -dumpmachine)
system_externalSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv6
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib