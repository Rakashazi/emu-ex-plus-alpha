LD = $(CC)

ifdef LINK_MAP
 MAPFILE := link.map
 LDFLAGS += -Wl,-Map=$(MAPFILE)
endif

ifdef O_RELEASE
 LDFLAGS += $(OPTIMIZE_LDFLAGS)
endif

ifdef PROFILE
 LDFLAGS += -pg
endif

LDFLAGS += $(EXTRA_LDFLAGS)
