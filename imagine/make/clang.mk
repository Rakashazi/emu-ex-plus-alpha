include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

CFLAGS_OPTIMIZE_DEBUG_DEFAULT ?= -O1

CFLAGS_WARN += -Wno-attributes

ifndef RELEASE
 CFLAGS_CODEGEN += -g
 ifndef compiler_noSanitizeAddress
  CFLAGS_CODEGEN += -fsanitize=address -fno-omit-frame-pointer
  ifndef O_LTO
   LDFLAGS += -fsanitize=address
  endif
 endif
endif

ifdef O_LTO
 CFLAGS_CODEGEN += -flto
endif

AR ?= llvm-ar

# TODO: check if still needed
ifeq ($(ENV),linux)
 CPPFLAGS += -D__extern_always_inline=inline
endif