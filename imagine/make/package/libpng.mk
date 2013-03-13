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
 else ifeq ($(CROSS_COMPILE), 1)
  CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config libpng --cflags --static --define-variable=prefix=$(system_externalSysroot))
  LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config libpng --libs --static --define-variable=prefix=$(system_externalSysroot))
 else
  CPPFLAGS += $(shell pkg-config libpng --cflags)
  # GOLD linker complains if -lz not explicitly linked
  LDLIBS += $(shell pkg-config libpng --libs) -lz
 endif
endif

endif