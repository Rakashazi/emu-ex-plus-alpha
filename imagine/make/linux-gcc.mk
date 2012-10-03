ENV := linux

ifeq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := clang
 endif
 include $(currPath)/clang.mk
else
 include $(currPath)/gcc.mk
endif

HIGH_OPTIMIZE_CFLAGS_MISC += -ffunction-sections -fdata-sections
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDFLAGS += -Wl,-O1,--gc-sections,--as-needed,--hash-style=gnu,--sort-common

CPPFLAGS += -I/usr/include/boost-1_50
ENV := linux
configDefs += CONFIG_ENV_LINUX
