include $(IMAGINE_PATH)/make/config.mk

compiler_sanitizeMode ?= address,undefined
include $(buildSysPath)/linux-gcc.mk

ARCH := x86_64
ifneq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := gcc-14
  CXX := g++-14
 endif
endif

CFLAGS_CODEGEN += -m64 -march=x86-64-v3 -mtune=generic
LDFLAGS_SYSTEM += -m64
ASMFLAGS += -m64