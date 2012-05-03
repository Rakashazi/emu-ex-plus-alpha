ENV := linux

ifeq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := clang
 endif
 include $(currPath)/clang.mk
else
 include $(currPath)/gcc.mk
endif

HIGH_OPTIMIZE_CFLAGS += -ffunction-sections -fdata-sections
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDFLAGS += -Wl,-O1,--gc-sections,--as-needed,--hash-style=gnu,--sort-common

ENV := linux
configDefs += CONFIG_ENV_LINUX
