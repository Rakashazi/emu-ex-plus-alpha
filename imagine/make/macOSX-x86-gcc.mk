include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/macOSX-gcc.mk

ARCH := x86
CPPFLAGS += -arch i386
LDFLAGS += -arch i386

PKG_CONFIG_PATH := $(IMAGINE_PATH)/bundle/macosx/x86/lib/pkgconfig:$(macportsPkgconfigPath)
