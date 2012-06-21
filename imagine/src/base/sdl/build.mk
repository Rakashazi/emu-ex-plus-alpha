ifndef inc_base
inc_base := 1

include $(IMAGINE_PATH)/make/package/sdl.mk

configDefs += CONFIG_BASE_SDL CONFIG_INPUT

SRC += base/sdl/main.cc

ifeq ($(webos_osVersion), 3)
 LDLIBS += -lpthread
endif

endif
