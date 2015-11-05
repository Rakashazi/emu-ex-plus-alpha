include $(IMAGINE_PATH)/make/config.mk
include $(buildSysPath)/linux-gcc.mk

ARCH := x86_64
ifeq ($(config_compiler),clang)
 CFLAGS_CODEGEN += -march=x86-64
else
 CFLAGS_CODEGEN += -m64 -mtune=generic
 LDFLAGS_SYSTEM += -m64
 ASMFLAGS += -m64
endif
