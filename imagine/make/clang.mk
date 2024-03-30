include $(buildSysPath)/common.mk
include $(buildSysPath)/gcc-link.mk
include $(buildSysPath)/gcc-common.mk

ifeq ($(origin AR), default)
 AR := llvm-ar
endif

ifndef RELEASE
 CFLAGS_CODEGEN += -g
endif

CFLAGS_CODEGEN += -fstrict-vtable-pointers

# needed for <ranges>
CPPFLAGS += -D_LIBCPP_ENABLE_EXPERIMENTAL

# needed for DelegateFuncSet.hh
CXXFLAGS_WARN += -Wno-vla-extension

ifeq ($(LTO_MODE),lto)
 ltoMode := lto
else ifeq ($(LTO_MODE),lto-fat)
 # lto-fat isn't supported
 ltoMode := lto
else ifeq ($(LTO_MODE),lto-link)
 # lto-link and off have the same effect
 ltoMode := lto-link
else
 ltoMode := lto-link
endif

ifeq ($(ltoMode),lto)
 CFLAGS_CODEGEN += -flto
 LDFLAGS_SYSTEM += $(CFLAGS_CODEGEN)
else
 # -flto needed to enable linking any static archives with LTO
 LDFLAGS_SYSTEM += -flto $(CFLAGS_CODEGEN)
endif
