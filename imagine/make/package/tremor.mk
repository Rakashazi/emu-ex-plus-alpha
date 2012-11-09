ifndef inc_pkg_tremor
inc_pkg_tremor := 1

configDefs += CONFIG_PACKAGE_TREMOR
ifeq ($(CROSS_COMPILE), 1)
 CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config vorbisidec --cflags --static --define-variable=prefix=$(system_externalSysroot))
 LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config vorbisidec --libs --static --define-variable=prefix=$(system_externalSysroot))
else
 CPPFLAGS += $(shell pkg-config vorbisidec --cflags)
 LDLIBS += $(shell pkg-config vorbisidec --libs)
endif

endif