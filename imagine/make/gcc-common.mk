#DEBUG := 1
#RELEASE := 1
#PROFILE := 1

CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT ?= -fomit-frame-pointer -fno-stack-protector
CFLAGS_OPTIMIZE_LEVEL_RELEASE_DEFAULT ?= -O2
CFLAGS_OPTIMIZE_RELEASE_DEFAULT ?= $(CFLAGS_OPTIMIZE_LEVEL_RELEASE_DEFAULT) $(CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT)
CFLAGS_CODEGEN += -pipe -fvisibility=hidden
CFLAGS_LANG = -std=gnu99 -fno-common
CXXFLAGS_LANG = -std=gnu++14 $(if $(cxxRTTI),,-fno-rtti) $(if $(cxxExceptions),,-fno-exceptions) \
$(if $(cxxThreadSafeStatics),,-fno-threadsafe-statics)

ifeq ($(ENV), android) # exceptions off by default on Android if using toolchain patches
 CXXFLAGS_LANG += $(if $(cxxExceptions),-fexceptions,)
endif

# setup warnings

CFLAGS_WARN ?= -Wall -Wextra -Werror=return-type -Wno-comment -Wno-missing-field-initializers -Wno-unused -Wno-unused-parameter
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
