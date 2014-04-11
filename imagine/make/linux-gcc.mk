ENV := linux

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
COMPILE_FLAGS += -ffunction-sections -fdata-sections
ifndef PROFILE
 OPTIMIZE_LDFLAGS = -s
endif
LDLIBS += -lm
LDFLAGS += -Wl,-O1,--gc-sections,--as-needed,--compress-debug-sections=zlib,--icf=all

