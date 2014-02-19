include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

WARNINGS_CFLAGS += -Wno-attributes

ifndef RELEASE
 COMPILE_FLAGS += -g
 ifndef compiler_noSanitizeAddress
  COMPILE_FLAGS += -fsanitize=address -fno-omit-frame-pointer
  ifndef O_LTO
   LDFLAGS += -fsanitize=address
  endif
 endif
endif

ifdef O_LTO
 COMPILE_FLAGS += -flto
 # TODO: does clang have fat LTO options?
 #ifndef O_LTO_FAT
 # COMPILE_FLAGS += -fno-fat-lto-objects
 #endif
endif

ifeq ($(origin AR), default)
 AR := llvm-ar
endif

ifeq ($(ENV),linux)
 CPPFLAGS += -D__extern_always_inline=inline
endif