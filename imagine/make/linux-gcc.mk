ENV := linux

ifndef target
 target = $(metadata_exec)
endif

ifndef O_RELEASE
 targetExtension := -debug
endif

ifdef staticLibcxx
 pkg_stdcxxStaticLib := -Wl,-Bstatic,-lstdc++,-Bdynamic
endif

ifeq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := clang
 endif
 include $(buildSysPath)/clang.mk
else
 include $(buildSysPath)/gcc.mk
endif

CPPFLAGS += -D_GNU_SOURCE
COMPILE_FLAGS += -ffunction-sections -fdata-sections
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDFLAGS += -Wl,-O1,--gc-sections,--as-needed,--hash-style=gnu,--compress-debug-sections=zlib,--icf=all

configDefs += CONFIG_ENV_LINUX
