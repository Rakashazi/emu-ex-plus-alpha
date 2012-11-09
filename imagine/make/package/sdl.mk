ifndef inc_pkg_sdl
inc_pkg_sdl := 1

LDLIBS += -lSDL

ifeq ($(ENV), webos)
 configDefs += CONFIG_BASE_SDL_PDL
 #CPPFLAGS += -D_GNU_SOURCE=1 -D_REENTRANT
 LDLIBS += -lpdl
else
 CPPFLAGS += -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
endif

endif