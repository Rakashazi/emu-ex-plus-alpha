include $(IMAGINE_PATH)/make/config.mk

ARCH := x86
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -x86
endif

minIOSVer = 5.0
IOS_FLAGS += -arch i386
ASMFLAGS += -arch i386
CFLAGS_CODEGEN += -mdynamic-no-pic -faligned-allocation
CHOST := $(shell $(CC) -arch i386 -dumpmachine)
