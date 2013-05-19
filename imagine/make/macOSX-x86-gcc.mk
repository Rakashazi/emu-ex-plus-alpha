include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(buildSysPath)/macOSX-gcc.mk

ARCH := x86
CPPFLAGS += -arch i386
LDFLAGS += -arch i386

CPPFLAGS += -I$(IMAGINE_PATH)/bundle/macosx-x86/include
LDLIBS += -L$(IMAGINE_PATH)/bundle/macosx-x86/lib
