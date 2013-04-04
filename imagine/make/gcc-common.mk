#DEBUG := 1
#RELEASE := 1
#PROFILE := 1
#O_NONE := 1
#O_SIZE := 1

COMPILE_FLAGS += -pipe -fvisibility=hidden
BASE_CFLAGS := -std=gnu99 -fno-common
BASE_CXXFLAGS = -std=gnu++11 -Woverloaded-virtual $(if $(cxxRTTI),,-fno-rtti) $(if $(cxxExceptions),,-fno-exceptions) \
$(if $(cxxThreadSafeStatics),,-fno-threadsafe-statics)

ifeq ($(ENV), android) # exceptions off by default on Android if using toolchain patches
 BASE_CXXFLAGS += $(if $(cxxExceptions),-fexceptions,)
endif

# setup warnings

ifndef NORMAL_WARNINGS_CFLAGS
 NORMAL_WARNINGS_CFLAGS = -Wall -Wextra -Wno-comment -Wno-missing-field-initializers
 #-Winline
endif

ifndef WARN_UNUSED
 WARNINGS_CFLAGS += $(NORMAL_WARNINGS_CFLAGS) -Wno-unused -Wno-unused-parameter
endif

# setup optimizations

ifdef O_RELEASE
 O_BEST := 1
 RELEASE := 1
endif

NORMAL_OPTIMIZE_CFLAGS_MISC := -ffast-math -fmerge-all-constants
ifdef RELEASE
 NORMAL_OPTIMIZE_CFLAGS_MISC += -fomit-frame-pointer
endif
HIGH_OPTIMIZE_CFLAGS_MISC = $(NORMAL_OPTIMIZE_CFLAGS_MISC)
#NORMAL_OPTIMIZE_CFLAGS_MISC += --param inline-unit-growth=1000 --param max-inline-insns-single=5000 --param large-function-growth=8000
NORMAL_OPTIMIZE_CFLAGS := -O2 $(NORMAL_OPTIMIZE_CFLAGS_MISC)
SIZE_OPTIMIZE_CFLAGS := -Os $(NORMAL_OPTIMIZE_CFLAGS_MISC)
ifneq ($(origin HIGH_OPTIMIZE_CFLAGS), file)
 HIGH_OPTIMIZE_CFLAGS := -O3 $(HIGH_OPTIMIZE_CFLAGS_MISC)
endif
WHOLE_PROGRAM_CFLAGS += -fwhole-program

ifdef O_SIZE
 OPTIMIZE_CFLAGS = $(SIZE_OPTIMIZE_CFLAGS)
else ifdef O_BEST
 OPTIMIZE_CFLAGS = $(HIGH_OPTIMIZE_CFLAGS)
else ifdef O_LOW
 OPTIMIZE_CFLAGS = -O1
else ifdef O_NONE
 OPTIMIZE_CFLAGS = 
else
 OPTIMIZE_CFLAGS = $(NORMAL_OPTIMIZE_CFLAGS)
endif

ifdef RELEASE
 ifndef LOGGER
  NO_LOGGER := 1
 endif
 CPPFLAGS += -DNDEBUG
 OPTIMIZE_CFLAGS += -fno-stack-protector
 WARNINGS_CFLAGS += -Wdisabled-optimization
endif

ifdef PROFILE
 COMPILE_FLAGS += -pg
endif

compileAction := -c

ifndef NO_SRC_DEPS
 compileAction += -MMD -MP
endif

COMPILE_FLAGS += $(OPTIMIZE_CFLAGS) $(EXTRA_CFLAGS)

# assign all the flags

CFLAGS = $(BASE_CFLAGS) $(COMPILE_FLAGS) $(WARNINGS_CFLAGS)
CXXFLAGS = $(BASE_CXXFLAGS) $(COMPILE_FLAGS) $(WARNINGS_CFLAGS) $(WARNINGS_CXXFLAGS)

AS := $(CC) -c
ASMFLAGS := -O3
