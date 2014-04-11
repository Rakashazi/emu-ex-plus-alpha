include $(IMAGINE_PATH)/make/config.mk

ARCH := arm
SUBARCH := armv6
MACHINE := GENERIC_ARMV6
include $(buildSysPath)/iOS-gcc.mk

# don't use clang++ since we're providing libc++
CXX := $(CC)

ifndef targetSuffix
 targetSuffix := -armv6
endif

IOS_SYSROOT = /Applications/Xcode44.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.1.sdk
IOS_FLAGS += -arch armv6
ASMFLAGS += -arch armv6
COMPILE_FLAGS += -mdynamic-no-pic
CHOST := $(shell $(CC) -arch armv6 -dumpmachine)

openGLESVersion := 1
