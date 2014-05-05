ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk
include $(imagineSrcDir)/input/build.mk
include $(IMAGINE_PATH)/make/package/sdl.mk

configDefs += CONFIG_BASE_SDL

SRC += base/sdl/main.cc input/genericASCIIDecode.cc

ifeq ($(webos_osVersion), 3)
 LDLIBS += -lpthread
endif

endif
