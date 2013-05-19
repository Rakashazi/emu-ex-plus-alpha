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
 else
  pkgConfigDeps += freetype2
 endif
endif

endif