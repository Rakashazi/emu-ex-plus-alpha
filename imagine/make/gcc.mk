include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

CFLAGS_OPTIMIZE_DEBUG_DEFAULT ?= -Og
CFLAGS_OPTIMIZE_RELEASE_DEFAULT += -funsafe-loop-optimizations

ifdef O_LTO
 CFLAGS_CODEGEN += -flto
 ifdef O_LTO_FAT
  CFLAGS_CODEGEN += -ffat-lto-objects
 endif
endif

AR ?= $(CHOST_PREFIX)gcc-ar

ifdef O_LTO_LINK_ONLY
 # link thin LTO objects with non-LTO objects
 LDFLAGS += -flto $(CFLAGS_CODEGEN)
endif

gccVersion := $(shell $(CC) -dumpversion)
#gcc_isAtLeastVer4_9 := $(shell expr $(gccVersion) \>= 4.9)

ifndef RELEASE
 ifndef compiler_noSanitizeAddress
  CFLAGS_CODEGEN += -fsanitize=address -fno-omit-frame-pointer
  ifndef O_LTO
   LDFLAGS += -fsanitize=address
  endif
 endif
endif

CFLAGS_WARN += $(if $(ccNoStrictAliasing),,-Werror=strict-aliasing) -fmax-errors=15

ifdef RELEASE
 CFLAGS_CODEGEN += -fno-ident
 CXXFLAGS_LANG += -fno-enforce-eh-specs
 CFLAGS_WARN += -Wunsafe-loop-optimizations
endif

ifndef RELEASE
 CFLAGS_CODEGEN += -ggdb
endif

ifdef cxxExceptions
 CXXFLAGS_LANG += -fnothrow-opt
endif
