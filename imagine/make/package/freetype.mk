ifndef inc_pkg_freetype
inc_pkg_freetype := 1

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