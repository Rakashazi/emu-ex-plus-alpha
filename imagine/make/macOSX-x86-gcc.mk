include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/macOSX-gcc.mk

ARCH := x86
CPPFLAGS += -arch i386
LDFLAGS += -arch i386

CPPFLAGS += -I$(IMAGINE_PATH)/bundle/macosx-x86/usr/include
LDLIBS += -L$(IMAGINE_PATH)/bundle/macosx-x86/usr/lib
