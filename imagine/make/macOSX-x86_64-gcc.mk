include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/macOSX-gcc.mk

ARCH := x86_64
CPPFLAGS += -arch x86_64
LDFLAGS_SYSTEM += -arch x86_64

PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(macportsPkgconfigPath)
