ifndef inc_pkg_unzip
inc_pkg_unzip := 1

ifeq ($(CROSS_COMPILE), 1)
 CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config minizip --cflags --static --define-variable=prefix=$(system_externalSysroot))
 LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config minizip --libs --static --define-variable=prefix=$(system_externalSysroot))
else
 CPPFLAGS += $(shell pkg-config minizip --cflags)
 # GOLD linker complains if -lz not explicitly linked
 LDLIBS += $(shell pkg-config minizip --libs) -lz
endif

endif