ifndef inc_pkg_egl
inc_pkg_egl := 1

ifeq ($(SUBENV), pandora)
 LDLIBS += -lEGL
else ifeq ($(ENV), android)
 LDLIBS += -lEGL
else ifeq ($(ENV), linux)
 pkgConfigDeps += egl
endif

endif