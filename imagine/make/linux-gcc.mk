ENV := linux

ifndef target
 target = $(metadata_exec)
endif

ifndef O_RELEASE
 targetExtension := -debug
endif

ifeq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := clang
 endif
 include $(currPath)/clang.mk
else
 include $(currPath)/gcc.mk
endif

CPPFLAGS += -D_GNU_SOURCE
HIGH_OPTIMIZE_CFLAGS_MISC += -ffunction-sections -fdata-sections
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDFLAGS += -Wl,-O1,--gc-sections,--as-needed,--hash-style=gnu

configDefs += CONFIG_ENV_LINUX
