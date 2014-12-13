include $(IMAGINE_PATH)/make/config.mk

ARCH := x86
MACHINE := GENERIC_X86
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -x86
endif

minIOSVer = 5.0
IOS_FLAGS += -arch i386
ASMFLAGS += -arch i386
COMPILE_FLAGS += -mdynamic-no-pic
CHOST := $(shell $(CC) -arch i386 -dumpmachine)

openGLESVersion ?= 2