include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

ifeq ($(origin AR), default)
 AR := $(CHOST_PREFIX)gcc-ar
endif

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
 CFLAGS_CODEGEN += -flto=auto -fuse-linker-plugin
else ifeq ($(ltoMode),lto-fat)
 CFLAGS_CODEGEN += -flto=auto -ffat-lto-objects -fuse-linker-plugin
else ifeq ($(ltoMode),lto-link)
 # link thin LTO objects with non-LTO objects
 LDFLAGS_SYSTEM += -flto=auto
else
 LDFLAGS_SYSTEM += -fno-lto
endif

CFLAGS_WARN += -fmax-errors=15

ifdef RELEASE
 CXXFLAGS_LANG += -fno-enforce-eh-specs
endif

ifndef RELEASE
 CFLAGS_CODEGEN += -g
endif

ASMFLAGS += -O3
