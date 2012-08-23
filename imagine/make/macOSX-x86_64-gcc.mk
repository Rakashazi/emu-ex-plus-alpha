include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/macOSX-gcc.mk

ARCH := x86_64
CPPFLAGS += -arch x86_64
LDFLAGS += -arch x86_64

CPPFLAGS += -I$(IMAGINE_PATH)/bundle/macosx-x86_64/usr/include
LDLIBS += -L$(IMAGINE_PATH)/bundle/macosx-x86_64/usr/lib
