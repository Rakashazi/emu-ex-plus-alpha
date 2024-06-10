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
 include $(buildSysPath)/clang.mk
 ifeq ($(origin CC), default)
  CC := clang
  CXX := clang++
  CXXFLAGS_LANG += -stdlib=libc++
  LDFLAGS_SYSTEM += -stdlib=libc++
 endif
else
 include $(buildSysPath)/gcc.mk
endif

CPPFLAGS += -D_GNU_SOURCE
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDLIBS += -lpthread -lm
LDFLAGS_SYSTEM += -Wl,-O3,--gc-sections,--as-needed,--compress-debug-sections=$(COMPRESS_DEBUG_SECTIONS),--icf=all

LDFLAGS_SYSTEM += -fuse-ld=mold