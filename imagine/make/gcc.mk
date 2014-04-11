include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

ifdef O_LTO
 COMPILE_FLAGS += -flto
 #COMPILE_FLAGS += -fipa-pta
 ifndef O_LTO_FAT
  COMPILE_FLAGS += -fno-fat-lto-objects
  ifeq ($(origin AR), default)
   # must use gcc's ar wrapper or slim-LTO won't work if building a static archive
   AR := $(CHOST_PREFIX)gcc-ar
  endif
 endif
endif

ifeq ($(origin AR), default)
 AR := $(CHOST_PREFIX)ar
endif

ifdef O_LTO_LINK_ONLY
 # link thin LTO objects with non-LTO objects
 LDFLAGS += -flto $(COMPILE_FLAGS)
endif

gccVersion := $(shell $(CC) -dumpversion)
#gcc_isAtLeastVer4_9 := $(shell expr $(gccVersion) \>= 4.9)

ifndef RELEASE
 ifndef compiler_noSanitizeAddress
  COMPILE_FLAGS += -fsanitize=address -fno-omit-frame-pointer
  ifndef O_LTO
   LDFLAGS += -fsanitize=address
  endif
 endif
endif

NORMAL_WARNINGS_CFLAGS += $(if $(ccNoStrictAliasing),,-Werror=strict-aliasing) -fmax-errors=15
#NORMAL_WARNINGS_CFLAGS += -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn

ifdef RELEASE
 COMPILE_FLAGS += -fno-ident
 BASE_CXXFLAGS += -fno-enforce-eh-specs
endif

ifndef RELEASE
 ifneq ($(ENV), ps3)
  COMPILE_FLAGS += -ggdb
 endif
endif

ifdef cxxExceptions
 BASE_CXXFLAGS += -fnothrow-opt
endif

HIGH_OPTIMIZE_CFLAGS_MISC += -funsafe-loop-optimizations
#-Wunsafe-loop-optimizations
ifndef gcc_noGraphite
# reduces performance in some cases, re-test with GCC 4.9
# HIGH_OPTIMIZE_CFLAGS_MISC += -floop-interchange -floop-strip-mine -floop-block
endif
HIGH_OPTIMIZE_CFLAGS := -O2 $(HIGH_OPTIMIZE_CFLAGS_MISC)
