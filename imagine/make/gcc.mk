include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

ifeq ($(origin AR), default)
 AR := $(CHOST_PREFIX)gcc-ar
endif

CFLAGS_OPTIMIZE_DEBUG_DEFAULT ?= -Og
CFLAGS_OPTIMIZE_MISC_RELEASE_DEFAULT += -funsafe-loop-optimizations -fno-ident

# Four possible LTO_MODE values when using GCC
ifeq ($(LTO_MODE),lto)
 ltoMode := lto
else ifeq ($(LTO_MODE),lto-fat)
 ltoMode := lto-fat
else ifeq ($(LTO_MODE),lto-link)
 ltoMode := lto-link
else
 ltoMode := off
endif

ifeq ($(ltoMode),lto)
 CFLAGS_CODEGEN += -flto
else ifeq ($(ltoMode),lto-fat)
 CFLAGS_CODEGEN += -flto -ffat-lto-objects
else ifeq ($(ltoMode),lto-link)
 # link thin LTO objects with non-LTO objects
 LDFLAGS_SYSTEM += -flto
else
 LDFLAGS_SYSTEM += -fno-lto
endif

CFLAGS_WARN += $(if $(ccNoStrictAliasing),,-Werror=strict-aliasing) -fmax-errors=15

ifdef RELEASE
 CXXFLAGS_LANG += -fno-enforce-eh-specs
 CFLAGS_WARN += -Wunsafe-loop-optimizations
endif

ifndef RELEASE
 CFLAGS_CODEGEN += -g3
 # When using GCC 11.1 with -fsanitize=undefined and -O1/-Og, all lambdas will produce
 # spurious null `this` pointer warnings, add interprocedural optimzation flags to prevent this
 # TODO: remove when GCC bug is fixed
 CFLAGS_CODEGEN += -fipa-cp -fipa-sra
endif

ASMFLAGS += -O3
