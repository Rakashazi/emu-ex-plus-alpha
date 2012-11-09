ifndef inc_pkg_dbus
inc_pkg_dbus := 1

ifeq ($(CROSS_COMPILE), 1)
 CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config dbus-1 --cflags --static --define-variable=prefix=$(system_externalSysroot))
 LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config dbus-1 --libs --static --define-variable=prefix=$(system_externalSysroot))
else
 CPPFLAGS += $(shell pkg-config dbus-1 --cflags)
 LDLIBS += $(shell pkg-config dbus-1 --libs)
endif

endif