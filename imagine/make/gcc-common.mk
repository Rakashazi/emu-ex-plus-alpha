#DEBUG := 1
#RELEASE := 1
#PROFILE := 1

CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT ?= -fomit-frame-pointer -fno-stack-protector -fno-asynchronous-unwind-tables
CFLAGS_OPTIMIZE_LEVEL_RELEASE_DEFAULT ?= -Ofast
CFLAGS_OPTIMIZE_RELEASE_DEFAULT ?= $(CFLAGS_OPTIMIZE_LEVEL_RELEASE_DEFAULT) $(CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT)
CFLAGS_CODEGEN += -pipe -fvisibility=hidden
CFLAGS_LANG = -fno-common
CXXFLAGS_LANG = -std=gnu++20 $(if $(cxxRTTI),,-fno-rtti) $(if $(cxxExceptions),,-fno-exceptions) \
$(if $(cxxThreadSafeStatics),,-fno-threadsafe-statics)

ifeq ($(ENV), ios)
 ifeq ($(SUBARCH), armv7)
  CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT := $(filter-out -fomit-frame-pointer, $(CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT))
 endif
endif

# setup warnings

CFLAGS_WARN ?= -Wall -Wextra -Werror=return-type -Wno-comment -Wno-unused -Wno-unused-parameter
CFLAGS_WARN += $(CFLAGS_WARN_EXTRA)
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

ifndef RELEASE
 ifeq ($(compiler_sanitizeMode), address)
  CFLAGS_CODEGEN += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
  LDFLAGS_SYSTEM += -fsanitize=address -fsanitize=undefined
  # Disable debug section compression since it may prevent symbols from appearing in backtrace
  COMPRESS_DEBUG_SECTIONS = none
 endif
 ifeq ($(compiler_sanitizeMode), thread)
  CFLAGS_CODEGEN += -fsanitize=thread -fno-omit-frame-pointer
  LDFLAGS_SYSTEM += -fsanitize=thread
  # Disable debug section compression since it may prevent symbols from appearing in backtrace
  COMPRESS_DEBUG_SECTIONS = none
 endif
endif

COMPRESS_DEBUG_SECTIONS ?= zlib
