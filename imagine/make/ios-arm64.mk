include $(IMAGINE_PATH)/make/config.mk

ARCH := aarch64
SUBARCH = arm64
MACHINE := GENERIC_AARCH64
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -arm64
endif

IOS_FLAGS += -arch arm64
ASMFLAGS += -arch arm64
CHOST := $(shell $(CC) -arch arm64 -dumpmachine)
# use aarch64 in host string to make configure scripts for packages happy
CHOST := $(subst arm64,aarch64,$(CHOST))

minIOSVer := 7.0
openGLESVersion ?= 2
