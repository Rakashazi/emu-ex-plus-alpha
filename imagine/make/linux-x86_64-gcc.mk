include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/linux-gcc.mk

ARCH := x86_64
ifeq ($(config_compiler),clang)
 COMPILE_FLAGS += -march=x86-64
else
 COMPILE_FLAGS += -m64 -mtune=generic
 LDFLAGS += -m64
 ASMFLAGS += -m64
endif

ifdef extraSysroot
 PKG_CONFIG_PATH := $(extraSysroot)/lib/pkgconfig
 CPPFLAGS += -I$(extraSysroot)/include
endif

linuxEventLoop := glib