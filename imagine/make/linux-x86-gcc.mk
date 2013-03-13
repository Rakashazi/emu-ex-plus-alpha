include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/linux-gcc.mk

ARCH := x86
COMPILE_FLAGS += -m32
LDFLAGS += -m32
ASMFLAGS += -m32

system_externalSysroot := $(IMAGINE_PATH)/bundle/linux-x86
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib
package_libpng_externalPath := $(system_externalSysroot)
