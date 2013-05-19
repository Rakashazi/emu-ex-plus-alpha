include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

ifdef O_LTO
 COMPILE_FLAGS += -flto
 LDFLAGS += $(COMPILE_FLAGS) $(WHOLE_PROGRAM_CFLAGS)
endif

gccVersion := $(shell $(CC) -dumpversion)
gcc_isAtLeastVer4_8 := $(shell expr $(gccVersion) \>= 4.8)

ifndef RELEASE
 ifndef compiler_noSanitizeAddress
  ifeq ($(gcc_isAtLeastVer4_8), 1)
   COMPILE_FLAGS += -fsanitize=address -fno-omit-frame-pointer
   ifndef O_LTO
    LDFLAGS += -fsanitize=address
   endif
  endif
 endif
endif

#WHOLE_PROGRAM_CFLAGS += -fipa-pta
NORMAL_WARNINGS_CFLAGS += $(if $(ccNoStrictAliasing),,-Werror=strict-aliasing)
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

HIGH_OPTIMIZE_CFLAGS_MISC += -funsafe-loop-optimizations -Wunsafe-loop-optimizations
ifndef gcc_noGraphite
 HIGH_OPTIMIZE_CFLAGS_MISC += -floop-interchange -floop-strip-mine -floop-block
endif
HIGH_OPTIMIZE_CFLAGS := -O2 $(HIGH_OPTIMIZE_CFLAGS_MISC)
