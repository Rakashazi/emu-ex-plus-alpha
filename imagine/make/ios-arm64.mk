include $(IMAGINE_PATH)/make/config.mk

ARCH := aarch64
SUBARCH = arm64
minIOSVer ?= 7.0
include $(buildSysPath)/iOS-gcc.mk

ifndef targetSuffix
 targetSuffix := -arm64
endif

IOS_FLAGS += -arch arm64
ASMFLAGS += -arch arm64
# TODO: remove when min iOS target is above 11.0
CFLAGS_CODEGEN += -faligned-allocation
CHOST := $(shell $(CC) -arch arm64 -dumpmachine)
# use aarch64 in host string to make configure scripts for packages happy
CHOST := $(subst arm64,aarch64,$(CHOST))
