ENV := linux
ENV_KERNEL := linux

ifndef target
 target = $(metadata_exec)
endif

ifndef O_RELEASE
 targetExtension := -debug
endif

ifdef staticLibcxx
 pkg_stdcxxStaticLib := -static-libstdc++
endif

ifeq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := clang
  CXX := clang++
 endif
 include $(buildSysPath)/clang.mk
else
 include $(buildSysPath)/gcc.mk
endif

CPPFLAGS += -D_GNU_SOURCE
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDLIBS += -lm
LDFLAGS_SYSTEM += -fuse-ld=mold -Wl,-O3,--gc-sections,--as-needed,--compress-debug-sections=$(COMPRESS_DEBUG_SECTIONS),--icf=all
