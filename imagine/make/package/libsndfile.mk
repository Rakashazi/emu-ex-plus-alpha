ifndef inc_pkg_libsndfile
inc_pkg_libsndfile := 1

ifdef package_libsndfile_externalPath
 CPPFLAGS += -I$(package_libsndfile_externalPath)/include
 LDLIBS += $(package_libsndfile_externalPath)/lib/libsndfile.a
else
 ifeq ($(CROSS_COMPILE), 1)
  CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config sndfile --cflags --static --define-variable=prefix=$(system_externalSysroot))
  LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config sndfile --libs --static --define-variable=prefix=$(system_externalSysroot))
 else
  CPPFLAGS += $(shell pkg-config sndfile --cflags)
  LDLIBS += $(shell pkg-config sndfile --libs)
 endif
endif

endif