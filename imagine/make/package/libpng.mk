ifndef inc_pkg_libpng
inc_pkg_libpng := 1

ifdef package_libpng_externalPath
 CPPFLAGS +=  -I$(package_libpng_externalPath)/include/libpng15
 LDLIBS += -L$(package_libpng_externalPath)/lib

 LDLIBS +=  -lpng15 -lz
else
 ifeq ($(ENV), webos)
  CPPFLAGS +=  -I$(WEBOS_PDK_PATH)/include/libpng12
  LDLIBS +=  -lpng12 -lz $(webos_libm)
 else ifeq ($(ENV), macosx)
  # MacPorts version
  LDLIBS += /opt/local/lib/libpng15.a -lz
 else
  pkgConfigDeps += libpng
 endif
endif

endif