LD = $(CXX)

#LINK_MAP=1
ifdef LINK_MAP
 MAPFILE := link.map
 LDFLAGS += -Wl,-Map=$(MAPFILE)
endif

ifdef O_RELEASE
 LDFLAGS_SYSTEM += $(OPTIMIZE_LDFLAGS)
endif

ifdef PROFILE
 LDFLAGS_SYSTEM += -pg
endif

linkLoadableModuleAction ?= -shared
loadableModuleExt := .so

LDFLAGS += $(CFLAGS_TARGET) $(LDFLAGS_SYSTEM) $(EXTRA_LDFLAGS)
