include $(currPath)/common.mk
include $(currPath)/gcc-link.mk
include $(currPath)/gcc-common.mk

BASE_CXXFLAGS += -std=gnu++0x
WARNINGS_CFLAGS += -Wno-unused-parameter -Wno-attributes -Wno-delete-non-virtual-dtor

ifndef RELEASE
 COMPILE_FLAGS += -g
endif

ifdef O_LTO
 COMPILE_FLAGS += -flto
 #OPTIMIZE_CFLAGS = -O4 $(NORMAL_OPTIMIZE_CFLAGS_MISC)
 LDFLAGS += $(COMPILE_FLAGS) $(WHOLE_PROGRAM_CFLAGS)
endif
