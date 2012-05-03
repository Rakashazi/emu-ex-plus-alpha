ENV := macOSX

ifeq ($(origin CC), default)
	CC := llvm-gcc
endif

include $(currPath)/gcc.mk

ifdef RELEASE
	COMPILE_FLAGS += -DNS_BLOCK_ASSERTIONS
endif

OSX_SYSROOT := /Developer/SDKs/MacOSX10.6.sdk
OSX_FLAGS = -isysroot $(OSX_SYSROOT) -mmacosx-version-min=10.4
CPPFLAGS += $(OSX_FLAGS)
LDFLAGS += $(OSX_FLAGS)
WARNINGS_CFLAGS += -Wno-attributes # for attributes not understood by llvm-gcc

COMPILE_FLAGS += -ftree-vectorize #-ftemplate-depth-100
LDFLAGS += -dead_strip -Wl,-S,-x
WHOLE_PROGRAM_CFLAGS := -fipa-pta -fwhole-program

system_externalSysroot := /opt/local
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib