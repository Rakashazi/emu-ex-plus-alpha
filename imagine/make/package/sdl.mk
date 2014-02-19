ifndef inc_pkg_sdl
inc_pkg_sdl := 1

ifeq ($(ENV), webos)
 configDefs += CONFIG_BASE_SDL_PDL
 #CPPFLAGS += -D_GNU_SOURCE=1 -D_REENTRANT
 LDLIBS += -lSDL -lpdl
else
 pkgConfigDeps += sdl
endif

endif