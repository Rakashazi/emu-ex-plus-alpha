LD = $(CC)

#LINK_MAP=1
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
