#DEBUG := 1
#RELEASE := 1
#PROFILE := 1

CFLAGS_OPTIMIZE_DEBUG_DEFAULT ?= -Og
CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT ?= -fomit-frame-pointer -fno-stack-protector -fno-asynchronous-unwind-tables
CFLAGS_OPTIMIZE_LEVEL_RELEASE_DEFAULT ?= -Ofast
CFLAGS_OPTIMIZE_RELEASE_DEFAULT ?= $(CFLAGS_OPTIMIZE_LEVEL_RELEASE_DEFAULT) $(CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT)
CFLAGS_CODEGEN += -pipe -fvisibility=hidden -ffunction-sections -fdata-sections
CFLAGS_LANG = -fno-common
CXXFLAGS_LANG = -std=gnu++26 $(if $(cxxThreadSafeStatics),,-fno-threadsafe-statics) -fvisibility-inlines-hidden

ifeq ($(ENV), ios)
 ifeq ($(SUBARCH), armv7)
  CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT := $(filter-out -fomit-frame-pointer, $(CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT))
 endif
endif

# setup warnings

CFLAGS_WARN ?= -Wall -Wextra -Werror=return-type -Wno-comment
CXXFLAGS_WARN ?= $(CFLAGS_WARN) -Woverloaded-virtual

# setup optimizations

ifdef O_RELEASE
 RELEASE := 1
endif

ifdef PROFILE
 RELEASE := 1
 CFLAGS_CODEGEN += -pg
endif

ifdef RELEASE
 CFLAGS_OPTIMIZE ?= $(CFLAGS_OPTIMIZE_RELEASE_DEFAULT)
 CPPFLAGS += -DNDEBUG
 CFLAGS_WARN += -Wdisabled-optimization
else
 CFLAGS_OPTIMIZE ?= $(CFLAGS_OPTIMIZE_DEBUG_DEFAULT)
endif

compileAction := -c

ifndef NO_SRC_DEPS
 compileAction += -MMD -MP
endif

CFLAGS_CODEGEN += $(CFLAGS_OPTIMIZE) $(CFLAGS_EXTRA)

# assign all the flags

CFLAGS = $(CFLAGS_LANG) $(CFLAGS_TARGET) $(CFLAGS_CODEGEN) $(CFLAGS_WARN)
CXXFLAGS = $(CXXFLAGS_LANG) $(CFLAGS_TARGET) $(CFLAGS_CODEGEN) $(CXXFLAGS_WARN)

AS = $(CC) -c

ifdef CHOST
 CHOST_PREFIX := $(CHOST)-
endif

# Disable some undefined sanitizers that greatly increase compile time or are not needed
compiler_noSanitizeMode ?= unreachable,return,vptr,enum,nonnull-attribute

ifndef RELEASE
 ifneq ($(compiler_sanitizeMode),)
  CFLAGS_CODEGEN += -fsanitize=$(compiler_sanitizeMode) -fno-omit-frame-pointer
  LDFLAGS_SYSTEM += -fsanitize=$(compiler_sanitizeMode)
  # Disable debug section compression since it may prevent symbols from appearing in backtrace
  COMPRESS_DEBUG_SECTIONS = none
  ifneq ($(compiler_sanitizeMode),)
   CFLAGS_CODEGEN += -fno-sanitize=$(compiler_noSanitizeMode)
   LDFLAGS_SYSTEM += -fno-sanitize=$(compiler_noSanitizeMode)
  endif
 endif
endif

COMPRESS_DEBUG_SECTIONS ?= zlib
