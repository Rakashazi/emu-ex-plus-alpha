ifndef inc_pkg_libpng
inc_pkg_libpng := 1

ifeq ($(ENV), webos)
 CPPFLAGS +=  -I$(WEBOS_PDK_PATH)/include/libpng12
 LDLIBS +=  -lpng12 -lz $(webos_libm)
else ifeq ($(ENV), macosx)
 # MacPorts version
 LDLIBS += /opt/local/lib/libpng.a -lz
else ifeq ($(libpngStatic), 1)
 pkgConfigStaticDeps += libpng
else
 pkgConfigDeps += libpng
endif

endif