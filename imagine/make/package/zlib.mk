ifndef inc_pkg_zlib
inc_pkg_zlib := 1

ifeq ($(ENV), android)
 LDLIBS += -lz
else ifeq ($(ENV), ios)
 LDLIBS += -lz
else ifeq ($(ENV), webos)
 LDLIBS += -lz
else
 pkgConfigDeps += zlib
endif

endif