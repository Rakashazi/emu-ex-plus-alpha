ifndef inc_pkg_freetype
inc_pkg_freetype := 1

ifdef package_freetype_externalPath
	CPPFLAGS +=  -I$(package_freetype_externalPath)/include
	LDLIBS += -L$(package_freetype_externalPath)/lib
else
 ifeq ($(ENV), webos)
  CPPFLAGS +=  -I$(WEBOS_PDK_PATH)/include/freetype2
  LDLIBS += -lfreetype
 else ifeq ($(ENV), macosx)
  # MacPorts version
  CPPFLAGS +=  -I/opt/local/include/freetype2
  LDLIBS += /opt/local/lib/libfreetype.a -lz
 else ifeq ($(CROSS_COMPILE), 1)
  CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config freetype2 --cflags --static --define-variable=prefix=$(system_externalSysroot))
  LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config freetype2 --libs --static --define-variable=prefix=$(system_externalSysroot))
 else
  CPPFLAGS += $(shell pkg-config freetype2 --cflags)
  LDLIBS += $(shell pkg-config freetype2 --libs)
 endif
endif

endif