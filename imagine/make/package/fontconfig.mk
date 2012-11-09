ifndef inc_pkg_fontconfig
inc_pkg_fontconfig := 1

configDefs += CONFIG_PACKAGE_FONTCONFIG

ifeq ($(CROSS_COMPILE), 1)
 CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config fontconfig --cflags --static --define-variable=prefix=$(system_externalSysroot))
 LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config fontconfig --libs --static --define-variable=prefix=$(system_externalSysroot))
else
 CPPFLAGS += $(shell pkg-config fontconfig --cflags)
 LDLIBS += $(shell pkg-config fontconfig --libs)
endif

endif