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
 #OPTIMIZE_CFLAGS = -O4 $(NORMAL_OPTIMIZE_CFLAGS_MISC)
 LDFLAGS += $(COMPILE_FLAGS) $(WHOLE_PROGRAM_CFLAGS)
endif

ifeq ($(ENV),linux)
 CPPFLAGS += -D__extern_always_inline=inline
endif